#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include "time.h"

#include "config.h"      // AP_SSID, AP_PASSWORD, REGISTER_URL, SECRET
#include "utils.h"       // generateIdentifier()
#include "index_html.h"  
#include "result_html.h" 

// ================== НАЛАШТУВАННЯ ТА ПІНИ ==================
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;
Servo myServo;

const int servoPin = 18; 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 10800;      // GMT+3
const int   daylightOffset_sec = 0;

#define FETCH_INTERVAL 60000UL 
String deviceId = "";
String targetTime = ""; 
bool apRunning = false;
unsigned long lastFetchTime = 0;

// Змінні для збереження стану серво
int currentServoAngle = 0;    
const int stepAngle = 58; // Крок повороту (змініть під свою кількість комірок)

// ================== ЛОГІКА СЕРВО ==================
void rotatePillDispenser() {
    Serial.println("💊 ЧАС ПРИЙОМУ! Поворот механізму...");
    
    currentServoAngle += stepAngle;

    // Скидання в нуль при досягненні ліміту (для стандартних серво 0-180)
    if (currentServoAngle > 180) {
        Serial.println("🔄 Коло завершено, повернення до 0°");
        currentServoAngle = 0;
    }

    myServo.write(currentServoAngle);
    
    Serial.print("✅ Зупинка на куті: ");
    Serial.println(currentServoAngle);
}

// ================== СИНХРОНІЗАЦІЯ ЧАСУ ==================
void checkTimeAndAlarm() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    char currentTime[6];
    strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);

    if (targetTime != "" && String(currentTime) == targetTime) {
        rotatePillDispenser();
        targetTime = ""; // Скидаємо до наступного оновлення
    }
}

// ================== ЗАПИТ ДО СЕРВЕРА ==================
void fetchSchedule() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        client->setInsecure();
        HTTPClient http;
        String fullUrl = String(REGISTER_URL) + "?device_id=" + deviceId + "&secret=" + SECRET;
        
        if (http.begin(*client, fullUrl)) {
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200) {
                String payload = http.getString();
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, payload);

                if (doc.containsKey("time")) {
                    targetTime = doc["time"].as<String>();
                } else if (doc["schedule"].size() > 0) {
                    targetTime = doc["schedule"][0]["times"][0].as<String>();
                }
                Serial.println("🕒 Отримано розклад на: " + targetTime);
            }
            http.end();
        }
        delete client;
    }
}

// ================== WIFI ПРЕРЕФЕНСИ ==================
bool loadCredentials(String &devId, String &ssid, String &pass) {
    prefs.begin("device", false);
    devId = prefs.getString("deviceId", "");
    ssid  = prefs.getString("ssid", "");
    pass  = prefs.getString("password", "");
    prefs.end();
    return (devId != "" && ssid != "");
}

void saveCredentials(const String &devId, const String &ssid, const String &pass) {
    prefs.begin("device", false);
    prefs.putString("deviceId", devId);
    prefs.putString("ssid", ssid);
    prefs.putString("password", pass);
    prefs.end();
}

// ================== ТОЧКА ДОСТУПУ (AP) ==================
void startAP() {
    apRunning = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    webServer.on("/", HTTP_GET, []() {
        webServer.send_P(200, "text/html", index_html);
    });

    webServer.on("/connect", HTTP_POST, []() {
        String ssid = webServer.arg("ssid");
        String pass = webServer.arg("password");
        if (deviceId == "") deviceId = generateIdentifier();

        WiFi.begin(ssid.c_str(), pass.c_str());
        int tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 30) { delay(500); tries++; }

        if (WiFi.status() == WL_CONNECTED) {
            saveCredentials(deviceId, ssid, pass);
            webServer.send(200, "text/html", getResultPage(deviceId, "SUCCESS"));
            delay(2000);
            ESP.restart(); // Перезавантаження для чистого входу в STA
        } else {
            webServer.send(200, "text/html", getResultPage(deviceId, "FAILED"));
        }
    });

    webServer.onNotFound([]() {
        webServer.sendHeader("Location", "http://192.168.4.1", true);
        webServer.send(302, "text/plain", "");
    });

    webServer.begin();
    Serial.println("📡 Точка доступу: http://192.168.4.1");
}

// ================== SETUP ==================
void setup() {
    Serial.begin(115200);
    myServo.attach(servoPin);
    myServo.write(currentServoAngle); // Початкова позиція

    String sSsid, sPass, sDevId;
    if (loadCredentials(sDevId, sSsid, sPass)) {
        // Якщо дані є - намагаємось підключитись
        deviceId = sDevId;
        Serial.print("📡 Спроба підключення до: "); Serial.println(sSsid);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(sSsid.c_str(), sPass.c_str());

        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 30) {
            delay(500);
            Serial.print(".");
            counter++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ WiFi підключено!");
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        } else {
            Serial.println("\n❌ Помилка підключення. Вмикаю AP...");
            startAP();
        }
    } else {
        // Даних немає - одразу вмикаємо AP
        Serial.println("ℹ️ Дані відсутні. Вмикаю режим налаштування...");
        startAP();
    }
}

// ================== LOOP ==================
void loop() {
    if (apRunning) {
        dnsServer.processNextRequest();
        webServer.handleClient();
    }

    if (WiFi.status() == WL_CONNECTED) {
        // Оновлення розкладу
        if (millis() - lastFetchTime > FETCH_INTERVAL) {
            fetchSchedule();
            lastFetchTime = millis();
        }
        
        // Перевірка часу кожну секунду
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 1000) {
            checkTimeAndAlarm();
            lastCheck = millis();
        }
    }
}