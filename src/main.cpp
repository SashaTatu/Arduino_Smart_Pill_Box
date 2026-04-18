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

#include "config.h"      
#include "utils.h"       
#include "index_html.h"  
#include "result_html.h" 

// ================== НАЛАШТУВАННЯ ТА ПІНИ ==================
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;
Servo myServo;

const int servoPin = 18; 
const int touchPin = 19;        
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800;      
const int  daylightOffset_sec = 0;

#define FETCH_INTERVAL 60000UL 
String deviceId = "";
String userId = "";
String targetTime = ""; 
bool apRunning = false;
unsigned long lastFetchTime = 0;

// Змінні стану
int currentServoAngle = 0;    
const int stepAngle = 45;       // Для 8 слотів: 360 / 8 = 45 градусів (якщо серво 360) 
                                // Або підберіть кут під ваш механізм
int currentSlot = 0;            // Змінна слоту (0-7)
bool waitingForConfirmation = false; 
bool wasExecutedToday = false; 
bool warningSentToday = false;



// ================== ВІДПРАВКА ЛОГУ НА СЕРВЕР ==================
void sendLogToServer(String eventName) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        client->setInsecure();
        HTTPClient http;
        
        // Формуємо URL
        String logUrl = String(LOG_URL);
        logUrl += "?user_id=" + userId;
        logUrl += "&event=" + eventName; // Використовуємо аргумент (dispensed або taken)
        logUrl += "&slot=" + String(currentSlot);
        logUrl += "&secret=" + String(SECRET);

        Serial.println("📤 Відправка логу (" + eventName + "): " + logUrl);

        if (http.begin(*client, logUrl)) {
            int httpResponseCode = http.GET();
            Serial.printf("📡 Відповідь сервера: %d\n", httpResponseCode);
            http.end();
        }
        delete client;
    }
}


String getWarningTime(String timeStr) {
    if (timeStr == "") return "";
    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();

    minutes -= 30;
    if (minutes < 0) {
        minutes += 60;
        hours -= 1;
        if (hours < 0) hours = 23;
    }

    char buf[6];
    sprintf(buf, "%02d:%02d", hours, minutes);
    return String(buf);
}


// ================== ЛОГІКА СЕРВО ТА КНОПКИ ==================

void rotatePillDispenser() {
    Serial.println("💊 ЧАС ПРИЙОМУ!");
    
    // 1. Збільшуємо номер слоту
    currentSlot++;
    if (currentSlot > 7) {
        currentSlot = 0; // Скидання після 7-го слоту
    }

    // 2. Логіка повороту серво
    currentServoAngle += stepAngle;
    if (currentServoAngle >= 180) { // Для стандартного серво 0-180
        currentServoAngle = 0;
    }

    myServo.write(currentServoAngle);
    
    Serial.print("✅ Перехід до слоту №: ");
    Serial.println(currentSlot);
    Serial.print("📐 Кут серво: ");
    Serial.println(currentServoAngle);

    sendLogToServer("open");

    waitingForConfirmation = true;
    wasExecutedToday = true; 
}

void handleTouchSensor() {
    if (waitingForConfirmation && digitalRead(touchPin) == HIGH) {
        Serial.print("🎯 Слот №");
        Serial.print(currentSlot);
        Serial.println(" підтверджено користувачем.");

        sendLogToServer("taken");

        waitingForConfirmation = false;
        delay(1000); 
    }
}

// ================== СИНХРОНІЗАЦІЯ ЧАСУ ==================
void checkTimeAndAlarm() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    char currentTime[6];
    strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);
    String currentStr = String(currentTime);

    if (targetTime == "") return;

    // --- ЛОГІКА ЗА 30 ХВИЛИН ---
    String warningTime = getWarningTime(targetTime);
    if (!warningSentToday && currentStr == warningTime) {
        Serial.println("🔔 Попередження: до прийому залишилось 30 хв");
        sendLogToServer("remind");
        warningSentToday = true; 
    }

    // --- ОСНОВНА ЛОГІКА ПРИЙОМУ ---
    if (!wasExecutedToday && !waitingForConfirmation && currentStr == targetTime) {
        rotatePillDispenser();
    }
}

// ================== ЗАПИТ ДО СЕРВЕРА ==================
void fetchSchedule() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        client->setInsecure();
        HTTPClient http;
        String fullUrl = String(SCHEDULE_URL) + "?device_id=" + deviceId + "&secret=" + SECRET;
        
        Serial.println("🌐 Запит до сервера...");
        
        if (http.begin(*client, fullUrl)) {
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200) {
                String payload = http.getString();
                Serial.println("📥 Отримано дані: " + payload);

                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);

                if (!error) {
                    // --- ВИТЯГУЄМО USER_ID ---
                    if (doc.containsKey("user_id")) {
                        userId = doc["user_id"].as<String>();
                        Serial.print("👤 User ID: ");
                        Serial.println(userId);
                    }

                    // --- ЛОГІКА ЧАСУ (залишається як була) ---
                    String newTime = "";
                    if (doc.containsKey("time")) {
                        newTime = doc["time"].as<String>();
                    } else if (doc.containsKey("schedule") && doc["schedule"].size() > 0) {
                        newTime = doc["schedule"][0]["times"][0].as<String>();
                    }

                    if (newTime != "" && newTime != targetTime) {
                        targetTime = newTime;
                        wasExecutedToday = false; 
                        Serial.println("⏰ Новий час встановлено: " + targetTime);
                    }
                } else {
                    Serial.print("❌ Помилка парсингу JSON: ");
                    Serial.println(error.c_str());
                }
            } else {
                Serial.print("⚠️ Помилка HTTP: ");
                Serial.println(httpResponseCode);
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
            ESP.restart();
        } else {
            webServer.send(200, "text/html", getResultPage(deviceId, "FAILED"));
        }
    });

    webServer.onNotFound([]() {
        webServer.sendHeader("Location", "http://192.168.4.1", true);
        webServer.send(302, "text/plain", "");
    });

    webServer.begin();
}

// ================== SETUP ==================
void setup() {
    Serial.begin(115200);
    delay(1000); // Даємо час порту ініціалізуватися
    Serial.println("\n--- ПРИСТРІЙ ЗАПУЩЕНО ---");
    pinMode(touchPin, INPUT); 
    
    myServo.setPeriodHertz(50);
    myServo.attach(servoPin, 500, 2400); 
    myServo.write(currentServoAngle);

    String sSsid, sPass, sDevId;
    if (loadCredentials(sDevId, sSsid, sPass)) {
        deviceId = sDevId;
        WiFi.mode(WIFI_STA);
        WiFi.begin(sSsid.c_str(), sPass.c_str());

        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 20) {
            delay(500);
            Serial.print(".");
            counter++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ WiFi підключено!");
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        } else {
            startAP();
        }
    } else {
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
        handleTouchSensor();

        if (millis() - lastFetchTime > FETCH_INTERVAL) {
            fetchSchedule();
            lastFetchTime = millis();
        }
        
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 1000) {
            checkTimeAndAlarm();
            lastCheck = millis();
        }

        // Скидання блокування повторного спрацювання
        struct tm timeinfo;
        if (wasExecutedToday && getLocalTime(&timeinfo)) {
            char currentTime[6];
            strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);
            if (String(currentTime) != targetTime) {
                wasExecutedToday = false; 
            }
        }
    }
}