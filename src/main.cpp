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
#include "DHT.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include "config.h"      
#include "utils.h"       
#include "index_html.h"  
#include "result_html.h" 

// ================== НАЛАШТУВАННЯ ТА ПІНИ ==================
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int servoPin = 18; 
const int touchPin = 19;
const int buzzerPin = 14;         
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
const int stepAngle = 45;       
int currentSlot = 0;            
bool waitingForConfirmation = false; 
bool wasExecutedToday = false; 

// --- ЧАСОВІ ПОРЕГИ ТА ПРАПОРЦІ ---
unsigned long dispenserActionTime = 0;    
bool reminderSent = false;                
bool missedSent = false;                  

const unsigned long REMINDER_DELAY = 2 * 60 * 1000; // 20 хвилин
const unsigned long MISSED_DELAY   = 6 * 60 * 1000; // 60 хвилин (1 година)


unsigned long previousMillis = 0; 
const long interval = 600000;

// ================== Temp and Hum ==================

#define DHTPIN 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Глобальні змінні для зберігання останніх показників
float humidity = 0;
float temperature = 0;


//==== іконки для LCD (можна розширити за потребою) ====
// Іконка термометра
byte termometerChar[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

// Іконка краплі (вологість)
byte humidityChar[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b10001,
  0b01110
};

// Іконка газу
byte gasChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01010,
  0b11111,
  0b00000
};

// ================== AIR ==================


#define MQ135_PIN 34

int airQuality = 0;

// ================== Музика ==================

void beep(int duration) {
    digitalWrite(buzzerPin, HIGH);
    delay(duration);
    digitalWrite(buzzerPin, LOW);
    delay(60);
}

void playSuccessSound() {
    beep(80);   // короткий старт
    delay(80);
    beep(80);   // підтвердження
    delay(80);
    beep(180);  // фінальний "вау"
    delay(120);
    beep(300);  // довгий завершальний сигнал
}


void playReminderSound() {
    for (int i = 0; i < 2; i++) {
        beep(120);
        delay(200);
        beep(120);
        delay(400);
    }
}

// ================== ВІДПРАВКА ЛОГУ НА СЕРВЕР ==================
void sendLogToServer(String eventName) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        client->setInsecure();
        HTTPClient http;
        
        String logUrl = String(LOG_URL);
        logUrl += "?user_id=" + userId;
        logUrl += "&event=" + eventName; 
        logUrl += "&slot=" + String(currentSlot);
        logUrl += "&secret=" + String(SECRET);

        Serial.println("📤 Відправка логу [" + eventName + "]: " + logUrl);

        if (http.begin(*client, logUrl)) {
            int httpResponseCode = http.GET();
            Serial.printf("📡 Відповідь сервера: %d\n", httpResponseCode);
            http.end();
        }
        delete client;
    }
}


void updateSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Перевірка, чи дані коректні
  if (isnan(h) || isnan(t)) {
    Serial.println("[-] Помилка зчитування з датчика DHT!");
    return; // Виходимо з функції, якщо помилка
  }

  // Оновлюємо значення
  humidity = h;
  temperature = t;
}


void readAirQuality() {
  int sensorValue = analogRead(MQ135_PIN); // Зчитуємо аналогове значення
  airQuality = sensorValue;
}


// =================== ВІДОБРАЖЕННЯ НА LCD ==================

void displayData() {
  lcd.clear();
  
  // Рядок 1: T та H
  lcd.setCursor(0, 0);
  
  lcd.write(0); // Термометр
  lcd.print(" ");
  lcd.print((int)temperature);
  lcd.write(223); // Градус
  lcd.print("C  ");
  
  lcd.write(1); // Крапля
  lcd.print(" ");
  lcd.print((int)humidity);
  lcd.print("%");

  // Рядок 2: Air Quality
  lcd.setCursor(0, 1);
  lcd.write(2); // Газ
  lcd.print(" ");
  
  if (airQuality < 100) lcd.print(" ");
  if (airQuality < 10) lcd.print(" ");
  lcd.print(airQuality);
  
  lcd.print(" [");
  if(airQuality > 700) {
    lcd.print("BAD]"); 
  } else if(airQuality > 400) {
    lcd.print("WRN]"); 
  } else {
    lcd.print(" OK]"); 
  }
}



void displayReminder() {
  lcd.clear();
  
  // Перший рядок: час прийому
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(targetTime); // Виводить ваш час, наприклад, 14:30
  
  // Додамо маленьку іконку годинника або знак оклику, якщо хочете
  lcd.setCursor(15, 0);
  lcd.print("!");

  // Другий рядок: заклик до дії
  lcd.setCursor(0, 1);
  lcd.print("TAKE YOUR PILL!");
}
// ================== ЛОГІКА СЕРВО ТА КНОПКИ ==================

void rotatePillDispenser() {
    Serial.println("💊 ЧАС ПРИЙОМУ!");
    
    // 1. Підключаємо серво безпосередньо перед рухом
    myServo.attach(servoPin, 500, 2400); 
    delay(50); // Даємо час на стабілізацію

    currentSlot++;
    if (currentSlot > 7) currentSlot = 0;

    currentServoAngle += stepAngle;
    if (currentServoAngle >= 180) currentServoAngle = 0;

    myServo.write(currentServoAngle);
    
    delay(500); // Даємо час серво фізично доїхати до точки
    
    // 2. ВІДКЛЮЧАЄМО СЕРВО (detach), щоб воно не смикалося далі
    myServo.detach(); 

    sendLogToServer("open");
    playSuccessSound();

    waitingForConfirmation = true;
    wasExecutedToday = true; 
    reminderSent = false;           
    missedSent = false;
    dispenserActionTime = millis(); 
}

void handleTouchSensor() {
    if (waitingForConfirmation && digitalRead(touchPin) == HIGH) {
        Serial.println("🎯 Прийом підтверджено користувачем.");
        sendLogToServer("taken");
        displayData(); // Повертаємо основний дисплей

        // Скасовуємо всі очікування
        waitingForConfirmation = false;
        reminderSent = true; 
        missedSent = true;
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

    if (targetTime == "" || wasExecutedToday) return;

    if (currentStr == targetTime) {
        rotatePillDispenser();
        displayReminder();
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
        
        if (http.begin(*client, fullUrl)) {
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200) {
                String payload = http.getString();
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);

                if (!error) {
                    if (doc.containsKey("user_id")) userId = doc["user_id"].as<String>();

                    String newTime = "";
                    if (doc.containsKey("time")) {
                        newTime = doc["time"].as<String>();
                    } else if (doc.containsKey("schedule") && doc["schedule"].size() > 0) {
                        newTime = doc["schedule"][0]["times"][0].as<String>();
                    }

                    if (newTime != "" && newTime != targetTime) {
                        targetTime = newTime;
                        wasExecutedToday = false; 
                        Serial.println("⏰ Новий час: " + targetTime);
                    }
                }
            }
            http.end();
        }
        delete client;
    }
}

// ================== WIFI PREFERENCES ==================
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

void startAP() {
    apRunning = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    webServer.on("/", HTTP_GET, []() { webServer.send_P(200, "text/html", index_html); });
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
            delay(2000); ESP.restart();
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
    pinMode(touchPin, INPUT); 
    pinMode(buzzerPin, OUTPUT);

    // 1. Спочатку ініціалізація дисплея
    Wire.begin(16, 17); // SDA, SCL
    lcd.init();
    lcd.backlight();
    lcd.clear();

    lcd.createChar(0, termometerChar);
    lcd.createChar(1, humidityChar);
    lcd.createChar(2, gasChar);


    lcd.setCursor(0, 0);
    lcd.print("System Booting...");

    // 2. Потім датчики
    dht.begin();
    updateSensorData();
    readAirQuality();
    
    // 3. Виводимо перші дані, щоб екран не був порожнім
    displayData();

    String sSsid, sPass, sDevId;
    if (loadCredentials(sDevId, sSsid, sPass)) {
        deviceId = sDevId;
        WiFi.mode(WIFI_STA);
        WiFi.begin(sSsid.c_str(), sPass.c_str());
        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 20) { delay(500); Serial.print("."); counter++; }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ WiFi OK");
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        } else { startAP(); }
    } else { startAP(); }
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

        // --- ЛОГІКА ТАЙМЕРІВ (Remind та Missed) ---
        if (waitingForConfirmation) {
            unsigned long timePassed = millis() - dispenserActionTime;

            // 1. Нагадування через 20 хвилин
            if (!reminderSent && timePassed >= REMINDER_DELAY) {
                Serial.println("🔔 Нагадування (20 хв)");
                sendLogToServer("remind");
                playReminderSound();
                reminderSent = true; 
            }

            // 2. Пропуск через 60 хвилин
            if (!missedSent && timePassed >= MISSED_DELAY) {
                Serial.println("⚠️ Пропущено (60 хв)");
                sendLogToServer("missed");
                missedSent = true; 
                
                // Опціонально: можна вимкнути waitingForConfirmation тут, 
                // якщо ми вважаємо, що після години підтвердження вже не актуальне.
                // waitingForConfirmation = false; 
            }
        }


        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            updateSensorData();
            readAirQuality();
            displayData(); // Тут всередині вже є lcd.clear()
        }

        // Скидання прапорця виконання для наступного дня/часу
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