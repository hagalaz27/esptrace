#include <WiFi.h>
#include <ESPTrace.h>

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASS";
const char* TOKEN     = "YOUR_TOKEN_FROM_ESPTRACE_COM";

ESPTrace logger(TOKEN);

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected!");

    // Log useful startup info
    logger.logResetReason();
    logger.logSystemInfo();
    logger.logWifiInfo();
    logger.info("Device started!");

    // Register remote command handlers
    logger.onCommand("led_on", [](const String& payload) {
        // digitalWrite(LED_PIN, HIGH);
        logger.info("LED turned ON");
    });

    logger.onCommand("led_off", [](const String& payload) {
        // digitalWrite(LED_PIN, LOW);
        logger.info("LED turned OFF");
    });
}

void loop() {
    // Check for remote commands every 3 seconds
    logger.checkCommands();

    // Log single sensor
    float temperature = 24.5;
    logger.logSensor("temperature", temperature, "C");

    // Log multiple sensors at once
    const char* names[]  = {"temp", "humidity", "voltage"};
    float       values[] = {24.5,   63.0,        3.3};
    logger.logSensors(names, values, 3);

    // Check if last send was successful
    if (!logger.lastSendOk()) {
        Serial.println("Log failed: " + logger.getLastError());
    }

    delay(5000);
}
