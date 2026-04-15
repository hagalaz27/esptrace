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

    logger.onCommand("led_on", [](const String& payload) {
        digitalWrite(LED_BUILTIN, HIGH);
        logger.info("LED turned ON");
    });

    // Starts background task on core 0 — won't block your code
    logger.startCommandListener();
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
| `log(msg, level)` | Log with custom level |
| `logJson(doc)` | Log ArduinoJson document |
| `logSensor(name, value, unit)` | Log single sensor |
| `logSensors(sensors[], count)` | Log multiple sensors |
| `logSystemInfo()` | Log heap, CPU, uptime |
| `logWifiInfo()` | Log IP, MAC, RSSI |
| `logResetReason()` | Log ESP32 reset reason |
| `startCommandListener()` | Start background command task (core 0) |
| `onCommand(cmd, callback)` | Register command handler |
| `setCommandInterval(ms)` | Set command poll interval (default 3000ms) |
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

## Sensors

Single sensor, unit is optional:

```cpp
logger.logSensor("temperature", 24.5);
logger.logSensor("temperature", 24.5, "C");
```

Both produce consistent JSON format:
```json
{"temperature": {"value": 24.5}}
{"temperature": {"value": 24.5, "unit": "C"}}
```

Multiple sensors using `ESPTrace::Sensor` struct:

```cpp
// With units
ESPTrace::Sensor sensors[] = {
    {"temp",     24.5, "C"},
    {"humidity", 63.0, "%"},
    {"voltage",  3.3,  "V"},
    {"uptime",   3600},       // unit is optional
};
logger.logSensors(sensors, 4);
```

## Remote Commands

Commands run in a **background FreeRTOS task on core 0** — your main code on core 1 is never blocked.

```cpp
void setup() {
    // Register handlers
    logger.onCommand("led_on", [](const String& payload) {
        digitalWrite(LED_BUILTIN, HIGH);
        logger.info("LED turned ON");
    });

    logger.onCommand("led_off", [](const String& payload) {
        digitalWrite(LED_BUILTIN, LOW);
        logger.info("LED turned OFF");
    });

    logger.onCommand("set_interval", [](const String& payload) {
        logger.setCommandInterval(payload.toInt());
    });

    // Optional: change polling interval (default 3000ms)
    logger.setCommandInterval(5000);

    // Start background listener — call once in setup()
    logger.startCommandListener();
}

void loop() {
    // No checkCommands() needed here
}
```

Built-in commands handled automatically:
- `reboot` — restarts the device
- `ping` — responds with `info("Pong!")`

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

// Disable entirely (e.g. before deep sleep)
logger.enable(false);
```

## Dependencies
- [ArduinoJson](https://arduinojson.org/) v7+

## License
MIT