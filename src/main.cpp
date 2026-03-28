#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"      // Містить AP_SSID та AP_PASSWORD
#include "utils.h"       // Містить функцію generateIdentifier()
#include "index_html.h"  // HTML код сторінки налаштувань
#include "result_html.h" // HTML код сторінки результату

// ================== ОБ'ЄКТИ ТА КОНСТАНТИ ==================
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

#define AP_KEEP_TIME 120000UL // Час роботи AP після з'єднання (2 хв)
#define WIFI_TRIES 40         // Спроби підключення до WiFi

String deviceId = "";
bool apRunning = false;
unsigned long apStartTime = 0;

// ================== РОБОТА З FLASH (Preferences) ==================
bool loadCredentials(String &devId, String &ssid, String &pass) {
    prefs.begin("device", true); // true = тільки читання
    devId = prefs.getString("deviceId", "");
    ssid  = prefs.getString("ssid", "");
    pass  = prefs.getString("password", "");
    prefs.end();
    return !(devId.isEmpty() || ssid.isEmpty());
}

void saveCredentials(const String &devId, const String &ssid, const String &pass) {
    prefs.begin("device", false); // false = запис дозволено
    prefs.putString("deviceId", devId);
    prefs.putString("ssid", ssid);
    prefs.putString("password", pass);
    prefs.end();
}

// ================== ACCESS POINT (AP) ==================
void startAP() {
    if (apRunning) return;
    
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    // Запуск DNS сервера для Captive Portal (перехоплює запити на порт 53)
    dnsServer.start(53, "*", apIP);
    
    apRunning = true;
    apStartTime = millis();
    Serial.println("📡 AP Started: " + String(AP_SSID));
}

void stopAP() {
    if (!apRunning) return;
    dnsServer.stop();
    webServer.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
    Serial.println("🛑 AP Stopped");
}

// ================== CAPTIVE PORTAL & WEB ==================
void handleRoot() {
    // Віддаємо головну сторінку з index_html.h
    webServer.send_P(200, "text/html", index_html);
}

void handleNotFound() {
    // Ключова частина Captive Portal: перенаправлення будь-якого запиту на IP точку доступу
    webServer.sendHeader("Location", "http://" + apIP.toString(), true);
    webServer.send(302, "text/plain", "");
}

void handleConnect() {
    if (!webServer.hasArg("ssid") || !webServer.hasArg("password")) {
        webServer.send(400, "text/plain", "Missing credentials");
        return;
    }

    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("password");

    // ГЕНЕРАЦІЯ ID (якщо його ще немає)
    if (deviceId.isEmpty()) {
        deviceId = generateIdentifier(); // Функція з utils.h
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < WIFI_TRIES) {
        delay(250);
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        saveCredentials(deviceId, ssid, pass);
        // Віддаємо сторінку успіху
        webServer.send(200, "text/html", getResultPage(deviceId, "SUCCESS"));
    } else {
        webServer.send(200, "text/html", getResultPage(deviceId, "WiFi failed"));
    }
}

// ================== SETUP & LOOP ==================
void setup() {
    Serial.begin(115200);
    
    String savedSsid, savedPass, savedDevId;
    bool hasData = loadCredentials(savedDevId, savedSsid, savedPass);

    WiFi.mode(WIFI_AP_STA);

    if (!hasData) {
        startAP(); // Немає даних — вмикаємо точку доступу
    } else {
        deviceId = savedDevId;
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        startAP(); // Залишаємо AP активним на випадок, якщо треба змінити WiFi
    }

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/connect", HTTP_POST, handleConnect);
    webServer.onNotFound(handleNotFound); // Обробка для Captive Portal
    webServer.begin();
}

void loop() {
    webServer.handleClient();
    dnsServer.processNextRequest(); // Обов'язково для роботи Captive Portal

    // Автоматичне вимкнення AP через деякий час після вдалого підключення
    if (apRunning && WiFi.status() == WL_CONNECTED && (millis() - apStartTime > AP_KEEP_TIME)) {
        stopAP();
    }
}