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

    // Sensors without units
	ESPTrace::Sensor sensors[] = {
		{"temp",     24.5},
		{"humidity", 63.0},
		{"voltage",  3.3}
	};

	// Sensors with units
	ESPTrace::Sensor sensors[] = {
		{"temp",     24.5, "C"},
		{"humidity", 63.0, "%"},
		{"voltage",  3.3,  "V"}
	};

	// Sensors with and without units
	ESPTrace::Sensor sensors[] = {
		{"temp",     24.5, "C"},
		{"uptime",   3600},       // without unit
		{"voltage",  3.3,  "V"}
	};

	logger.logSensors(sensors, 3);

	// One sensor - same API
	logger.logSensor("temp", 24.5, "C");
	logger.logSensor("uptime", 3600); // without unit

    // Check if last send was successful
    if (!logger.lastSendOk()) {
        Serial.println("Log failed: " + logger.getLastError());
    }

    delay(5000);
}
