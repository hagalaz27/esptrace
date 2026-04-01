# ESPTrace

Remote debug logger for ESP32. Send logs over WiFi and view them in real-time at [esptrace.com](https://esptrace.com).

Especially useful when ESP32 is in **USB HID mode** and Serial monitor is unavailable.

## Installation

### PlatformIO
Add to `platformio.ini`:
```ini
lib_deps =
    https://github.com/hagalaz27/esptrace
    bblanchon/ArduinoJson @ ^7.0.0
```

### Arduino IDE
Sketch → Include Library → Add .ZIP Library → paste:
```
https://github.com/hagalaz27/esptrace/archive/refs/heads/main.zip
```

## Quick Start

```cpp
#include <WiFi.h>
#include <ESPTrace.h>

ESPTrace logger("YOUR_TOKEN_FROM_ESPTRACE_COM");

void setup() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    logger.logResetReason();
    logger.logWifiInfo();
    logger.info("Device started!");
}

void loop() {
    logger.info("Hello from ESP32");
    delay(5000);
}
```

## Methods

| Method | Description |
|--------|-------------|
| `info(msg)` | Log info message |
| `warning(msg)` | Log warning |
| `error(msg)` | Log error |
| `debug(msg)` | Log debug message |
| `logJson(doc)` | Log ArduinoJson document |
| `logSensor(name, value, unit)` | Log single sensor value |
| `logSensors(names[], values[], count)` | Log multiple sensors |
| `logSystemInfo()` | Log heap, CPU, uptime |
| `logWifiInfo()` | Log IP, MAC, RSSI |
| `logResetReason()` | Log ESP32 reset reason |
| `checkCommands()` | Poll for remote commands |
| `onCommand(cmd, callback)` | Register command handler |
| `setMinLevel(level)` | Set minimum log level |
| `enable(bool)` | Enable/disable logger |
| `lastSendOk()` | Check last send status |
| `getLastError()` | Get last error message |

## Log Levels

```cpp
logger.debug("Raw ADC: 2048");
logger.info("Sensor initialized");
logger.warning("Low battery: 3.1V");
logger.error("Sensor timeout!");
```

## JSON Auto-render

Send JSON and it renders as a formatted table in the dashboard:

```cpp
JsonDocument doc;
doc["temp"]     = 24.5;
doc["humidity"] = 63;
doc["voltage"]  = 3.3;
logger.logJson(doc);

// Or use built-in helpers
logger.logSensor("temperature", 24.5, "C");

const char* names[]  = {"temp", "humidity", "voltage"};
float       values[] = {24.5,   63.0,        3.3};
logger.logSensors(names, values, 3);
```

## Remote Commands

Send commands from the dashboard to your running device:

```cpp
logger.onCommand("led_on", [](const String& payload) {
    digitalWrite(LED_PIN, HIGH);
});

logger.onCommand("set_delay", [](const String& payload) {
    int ms = payload.toInt();
});

void loop() {
    logger.checkCommands(); // poll every loop iteration
    delay(3000);
}
```

## Startup Diagnostics

```cpp
void setup() {
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    logger.logResetReason(); // "Power on" / "Panic" / "Watchdog" etc.
    logger.logSystemInfo();  // free heap, CPU freq, flash size, uptime
    logger.logWifiInfo();    // IP, MAC, SSID, RSSI, channel
}
```

## Min Level Filter

```cpp
// In production — skip DEBUG logs
logger.setMinLevel(ESPTrace::LOG_INFO);

// Disable entirely
logger.enable(false);
```

## Dependencies
- [ArduinoJson](https://arduinojson.org/) v7+

## License
MIT
