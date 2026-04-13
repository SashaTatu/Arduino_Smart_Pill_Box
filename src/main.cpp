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

#include "config.h"      // AP_SSID, AP_PASSWORD
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
const long  gmtOffset_sec = 10800;      // GMT+3 (Літній час в Україні)
const int   daylightOffset_sec = 0;     // Вже враховано в офсеті

#define FETCH_INTERVAL 60000UL 
#define AP_KEEP_TIME 120000UL

String deviceId = "";
String targetTime = ""; // Час із БД
bool apRunning = false;
unsigned long apStartTime = 0;
unsigned long lastFetchTime = 0;

// ================== ЛОГІКА СЕРВО ==================
void rotatePillDispenser() {
    Serial.println("💊 ЧАС ПРИЙОМУ! Повертаю механізм...");
    myServo.write(90);  // Поворот для видачі
    delay(1000);
    myServo.write(0);   // Повернення
    Serial.println("✅ Видано.");
}

// ================== СИНХРОНІЗАЦІЯ ЧАСУ ==================
void checkTimeAndAlarm() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    char currentTime[6];
    strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);

    // Якщо поточний час збігається з часом із БД
    if (targetTime != "" && String(currentTime) == targetTime) {
        rotatePillDispenser();
        targetTime = ""; // Скидаємо до наступного оновлення з сервера
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

                // Підтримка обох структур JSON (пряма строка або масив schedule)
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

// ================== КЕРУВАННЯ WIFI ТА ПАМ'ЯТТЮ ==================
bool loadCredentials(String &devId, String &ssid, String &pass) {
    prefs.begin("device", false);
    devId = prefs.getString("deviceId", "");
    ssid  = prefs.getString("ssid", "");
    pass  = prefs.getString("password", "");
    prefs.end();
    return !(devId.isEmpty() || ssid.isEmpty());
}

void saveCredentials(const String &devId, const String &ssid, const String &pass) {
    prefs.begin("device", false);
    prefs.putString("deviceId", devId);
    prefs.putString("ssid", ssid);
    prefs.putString("password", pass);
    prefs.end();
}

void startAP() {
    if (apRunning) return;
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    apRunning = true;
    apStartTime = millis();
    Serial.println("📡 AP Активовано");
}

void stopAP() {
    if (!apRunning) return;
    dnsServer.stop();
    webServer.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
    Serial.println("🛑 AP Вимкнено");
}

void handleConnect() {
    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("password");
    if (deviceId.isEmpty()) deviceId = generateIdentifier();

    WiFi.begin(ssid.c_str(), pass.c_str());
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) { delay(250); tries++; }

    if (WiFi.status() == WL_CONNECTED) {
        saveCredentials(deviceId, ssid, pass);
        webServer.send(200, "text/html", getResultPage(deviceId, "SUCCESS"));
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    } else {
        webServer.send(200, "text/html", getResultPage(deviceId, "FAILED"));
    }
}

// ================== SETUP & LOOP ==================
void setup() {
    Serial.begin(115200);
    myServo.attach(servoPin);
    myServo.write(0);

    String savedSsid, savedPass, savedDevId;
    bool hasData = loadCredentials(savedDevId, savedSsid, savedPass);

    WiFi.mode(WIFI_AP_STA);
    if (hasData) {
        deviceId = savedDevId;
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
    startAP();

    webServer.on("/", HTTP_GET, []() { webServer.send_P(200, "text/html", index_html); });
    webServer.on("/connect", HTTP_POST, handleConnect);
    webServer.onNotFound([]() {
        webServer.sendHeader("Location", "http://192.168.4.1", true);
        webServer.send(302, "text/plain", "");
    });
    webServer.begin();
}

void loop() {
    webServer.handleClient();
    dnsServer.processNextRequest();

    if (apRunning && WiFi.status() == WL_CONNECTED && (millis() - apStartTime > AP_KEEP_TIME)) {
        stopAP();
    }

    if (WiFi.status() == WL_CONNECTED) {
        // Оновлення розкладу раз на хвилину
        if (millis() - lastFetchTime > FETCH_INTERVAL) {
            fetchSchedule();
            lastFetchTime = millis();
        }
        // Перевірка будильника щосекунди
        static unsigned long lastTimeCheck = 0;
        if (millis() - lastTimeCheck > 1000) {
            checkTimeAndAlarm();
            lastTimeCheck = millis();
        }
    }
}