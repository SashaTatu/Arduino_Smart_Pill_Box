#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "config.h"      // Має містити AP_SSID та AP_PASSWORD
#include "utils.h"       // Має містити generateIdentifier()
#include "index_html.h"  
#include "result_html.h" 

// ================== ОБ'ЄКТИ ТА КОНСТАНТИ ==================
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

#define AP_KEEP_TIME 120000UL 
#define WIFI_TRIES 40         
#define FETCH_INTERVAL 60000UL 



String deviceId = "";
bool apRunning = false;
unsigned long apStartTime = 0;
unsigned long lastFetchTime = 0;

// ================== ФУНКЦІЯ ЗАПИТУ РОЗКЛАДУ ==================
void fetchSchedule() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Створюємо безпечного клієнта для роботи з Render (HTTPS)
    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        client->setInsecure(); // Важливо для обходу перевірки SSL сертифіката

        HTTPClient http;
        String fullUrl = String(REGISTER_URL) + "?device_id=" + deviceId + "&secret=" + String(SECRET);
        
        Serial.println("📡 Запит до сервера: " + fullUrl);
        
        if (http.begin(*client, fullUrl)) {
            int httpResponseCode = http.GET();
            
            if (httpResponseCode == 200) {
                String payload = http.getString();
                Serial.println("📥 Відповідь: " + payload);

                // Використовуємо DynamicJsonDocument для обробки JSON
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);

                if (!error) {
                    // Отримуємо значення. Оператор | задає текст, якщо ключ відсутній
                    const char* uId = doc["user_id"] | (doc["userid"] | "невідомо");
                    const char* t = doc["time"] | "00:00";

                    Serial.println("-------------------------");
                    Serial.printf("👤 User ID: %s\n", uId);
                    Serial.printf("⏰ Час прийому: %s\n", t);
                    Serial.println("-------------------------");
                } else {
                    Serial.print("❌ Помилка JSON: ");
                    Serial.println(error.f_str());
                }
            } else {
                Serial.printf("❌ помилка HTTP: %d\n", httpResponseCode);
            }
            http.end();
        }
        delete client; // Звільняємо оперативну пам'ять
    }
}

// ================== РОБОТА З ПАМ'ЯТТЮ (NVS) ==================
bool loadCredentials(String &devId, String &ssid, String &pass) {
    prefs.begin("device", false); // false дозволяє створити розділ, якщо його немає
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

// ================== ТОЧКА ДОСТУПУ (AP) ==================
void startAP() {
    if (apRunning) return;
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", apIP);
    apRunning = true;
    apStartTime = millis();
    Serial.println("📡 AP активовано: " + String(AP_SSID));
}

void stopAP() {
    if (!apRunning) return;
    dnsServer.stop();
    webServer.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
    Serial.println("🛑 AP вимкнено для економії енергії");
}

// ================== WEB СЕРВЕР ТА ПОРТАЛ ==================
void handleRoot() {
    webServer.send_P(200, "text/html", index_html);
}

void handleNotFound() {
    webServer.sendHeader("Location", "http://" + apIP.toString(), true);
    webServer.send(302, "text/plain", "");
}

void handleConnect() {
    if (!webServer.hasArg("ssid") || !webServer.hasArg("password")) {
        webServer.send(400, "text/plain", "Помилка даних");
        return;
    }

    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("password");

    if (deviceId.isEmpty()) deviceId = generateIdentifier();

    Serial.println("🔄 Спроба підключення до: " + ssid);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < WIFI_TRIES) {
        delay(300);
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        saveCredentials(deviceId, ssid, pass);
        webServer.send(200, "text/html", getResultPage(deviceId, "SUCCESS"));
        Serial.println("✅ WiFi OK!");
    } else {
        webServer.send(200, "text/html", getResultPage(deviceId, "FAILED"));
        Serial.println("❌ Помилка WiFi");
    }
}

// ================== SETUP & LOOP ==================
void setup() {
    Serial.begin(115200);
    delay(500);

    String savedSsid, savedPass, savedDevId;
    bool hasData = loadCredentials(savedDevId, savedSsid, savedPass);

    WiFi.mode(WIFI_AP_STA);

    if (!hasData) {
        startAP();
    } else {
        deviceId = savedDevId;
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        startAP(); // Даємо можливість переналаштувати протягом AP_KEEP_TIME
    }

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/connect", HTTP_POST, handleConnect);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
}

void loop() {
    dnsServer.processNextRequest(); // Має викликатися якомога частіше
    webServer.handleClient();

    // Вимикаємо точку доступу через 2 хвилини після успішного підключення
    if (apRunning && WiFi.status() == WL_CONNECTED && (millis() - apStartTime > AP_KEEP_TIME)) {
        stopAP();
    }

    // Запит розкладу раз на хвилину
    if (WiFi.status() == WL_CONNECTED) {
        if (millis() - lastFetchTime > FETCH_INTERVAL) {
            fetchSchedule();
            lastFetchTime = millis();
        }
    }
}