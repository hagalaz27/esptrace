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
}

void loop() {
    logger.checkCommands(); // non-blocking, call every iteration
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
| `checkCommands()` | Poll remote commands (non-blocking) |
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

Single sensor with optional unit:

```cpp
logger.logSensor("temperature", 24.5);       // without unit
logger.logSensor("temperature", 24.5, "C");  // with unit
```

Multiple sensors using `ESPTrace::Sensor` struct:

```cpp
// Without units
ESPTrace::Sensor sensors[] = {
    {"temp",     24.5},
    {"humidity", 63.0},
    {"voltage",  3.3}
};

// With units
ESPTrace::Sensor sensors[] = {
    {"temp",     24.5, "C"},
    {"humidity", 63.0, "%"},
    {"voltage",  3.3,  "V"}
};

// Mixed — unit is optional per sensor
ESPTrace::Sensor sensors[] = {
    {"temp",     24.5, "C"},
    {"uptime",   3600},
    {"voltage",  3.3,  "V"}
};

logger.logSensors(sensors, 3);
```

## Remote Commands

Register handlers in `setup()`, call `checkCommands()` in `loop()` — no blocking, no manual delay needed:

```cpp
void setup() {
    logger.onCommand("led_on", [](const String& payload) {
        digitalWrite(LED_BUILTIN, HIGH);
        logger.info("LED turned ON");
    });

    logger.onCommand("led_off", [](const String& payload) {
        digitalWrite(LED_BUILTIN, LOW);
        logger.info("LED turned OFF");
    });

    logger.onCommand("set_interval", [](const String& payload) {
        int ms = payload.toInt();
        logger.setCommandInterval(ms);
    });

    // Change polling interval (default 3000ms)
    logger.setCommandInterval(5000);
}

void loop() {
    logger.checkCommands(); // internally throttled — safe to call every tick
    
    // rest of your code runs without blocking
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