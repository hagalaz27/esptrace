#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>

class ESPTrace {
public:

    // ── Constructor ───────────────────────────────────────────
    ESPTrace(const char* token, const char* serverUrl = "https://esptrace.com/api/log.php") {
        _token     = token;
        _serverUrl = serverUrl;
        _enabled   = true;
        _minLevel  = LOG_DEBUG;
    }

    struct Sensor {
        const char* name;
        float       value;
        const char* unit = "";
    };

    // ── Logging levels ─----───────────────────────────────────
    enum Level { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARNING = 2, LOG_ERROR = 3 };

    // ── Settings -─────────────────────────────────────────────

    // Enable/disable the logger
    void enable(bool state) { _enabled = state; }

    // Minimum level - logs below this level are ignored
    void setMinLevel(Level level) { _minLevel = level; }

    // Custom server URL
    void setServerUrl(const char* url) { _serverUrl = url; }

    // ── Basic logging methods ───────────────────────────

    bool debug(const String& message) {
        return send(message, "DEBUG");
    }

    bool info(const String& message) {
        return send(message, "INFO");
    }

    bool warning(const String& message) {
        return send(message, "WARNING");
    }

    bool error(const String& message) {
        return send(message, "ERROR");
    }

    // Universal method
    bool log(const String& message, const String& level = "INFO") {
        return send(message, level);
    }

    // ── Convenient methods for ESP32 ──────────────────────────────

    // Log the reason for the reboot
    bool logResetReason() {
        esp_reset_reason_t reason = esp_reset_reason();
        String msg = "Reset reason: " + resetReasonToString(reason);
        return send(msg, "WARNING");
    }

    // Log system information
    bool logSystemInfo() {
        JsonDocument doc;
        doc["free_heap"]    = ESP.getFreeHeap();
        doc["chip_model"]   = ESP.getChipModel();
        doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
        doc["flash_size"]   = ESP.getFlashChipSize();
        doc["uptime_sec"]   = millis() / 1000;
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // Log WiFi data
    bool logWifiInfo() {
        if (WiFi.status() != WL_CONNECTED) {
            return send("WiFi not connected", "WARNING");
        }
        JsonDocument doc;
        doc["ip"]      = WiFi.localIP().toString();
        doc["mac"]     = WiFi.macAddress();
        doc["ssid"]    = WiFi.SSID();
        doc["rssi"]    = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // Log an arbitrary JSON object
    bool logJson(JsonDocument& doc, const String& level = "INFO") {
        String json;
        serializeJson(doc, json);
        return send(json, level);
    }

    // Log sensor value
    bool logSensor(const char* name, float value, const char* unit = "") {
        Sensor s = {name, value, unit};
        return logSensors(&s, 1);
    }

    // Log multiple sensors at once
    bool logSensors(const Sensor sensors[], const int count) {
        JsonDocument doc;
        for (int i = 0; i < count; i++) {
            if (strlen(sensors[i].unit) > 0) {
                JsonObject obj = doc[sensors[i].name].to<JsonObject>();
                obj["value"] = sensors[i].value;
                obj["unit"]  = sensors[i].unit;
            } else {
                doc[sensors[i].name] = sensors[i].value;
            }
        }
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // ── Get the latest error ─────────────────────────────
    String getLastError() { return _lastError; }

    // ── Last shipment status ─────────────────────────────
    bool lastSendOk() { return _lastSendOk; }

    // Callback type for the command
    using CommandCallback = std::function<void(const String& payload)>;

    // Register a command handler
    void onCommand(const String& command, CommandCallback callback) {
        _commands[command] = callback;
    }

   // Check commands from the server
// Internally throttled — safe to call every loop() iteration
void startCommandListener() {
    xTaskCreatePinnedToCore(
        _commandTask,
        "ESPTraceCmd",
        8192,
        this,
        1,
        &_commandTaskHandle,
        0
    );
}

// Set command polling interval (default 3000ms)
void setCommandInterval(unsigned long ms) { _commandInterval = ms; }

private:
    const char* _token;
    const char* _serverUrl;
    bool        _enabled;
    Level       _minLevel;
    String      _lastError;
    bool        _lastSendOk = false;
    std::map<String, CommandCallback> _commands;
    unsigned long _lastCommandCheck = 0;
    unsigned long _commandInterval  = 3000;
	TaskHandle_t _commandTaskHandle = nullptr;

    // ── Sending to the server ────────────────────────────────────
    bool send(const String& message, const String& level) {
        if (!_enabled) return false;
        if (!levelAllowed(level)) return false;
        if (WiFi.status() != WL_CONNECTED) {
            _lastError  = "WiFi not connected";
            _lastSendOk = false;
            return false;
        }

        HTTPClient http;
        http.begin(_serverUrl);
        http.addHeader("Content-Type",   "application/json");
        http.addHeader("X-Device-Token", _token);
        http.setTimeout(5000);

        // Collecting JSON
        JsonDocument doc;
        doc["message"] = message;
        doc["level"]   = level;
        String body;
        serializeJson(doc, body);

        int code = http.POST(body);
        http.end();

        _lastSendOk = (code == 200);
        if (!_lastSendOk) _lastError = "HTTP " + String(code);
        return _lastSendOk;
    }

    bool levelAllowed(const String& level) {
        if (level == "DEBUG")   return _minLevel <= LOG_DEBUG;
        if (level == "INFO")    return _minLevel <= LOG_INFO;
        if (level == "WARNING") return _minLevel <= LOG_WARNING;
        if (level == "ERROR")   return _minLevel <= LOG_ERROR;
        return true;
    }

    String resetReasonToString(esp_reset_reason_t reason) {
        switch (reason) {
            case ESP_RST_POWERON:  return "Power on";
            case ESP_RST_EXT:      return "External pin";
            case ESP_RST_SW:       return "Software reset";
            case ESP_RST_PANIC:    return "Panic / exception";
            case ESP_RST_INT_WDT:  return "Interrupt watchdog";
            case ESP_RST_TASK_WDT: return "Task watchdog";
            case ESP_RST_WDT:      return "Watchdog";
            case ESP_RST_DEEPSLEEP:return "Wake from deep sleep";
            case ESP_RST_BROWNOUT: return "Brownout (low voltage)";
            case ESP_RST_SDIO:     return "SDIO reset";
            default:               return "Unknown (" + String(reason) + ")";
        }
    }
	
	static void _commandTask(void* param) {
		ESPTrace* self = (ESPTrace*)param;
		for (;;) {
			if (WiFi.status() == WL_CONNECTED) {
				HTTPClient http;
				String url = String(self->_serverUrl);
				url.replace("log.php", "command.php");
				http.begin(url);
				http.addHeader("X-Device-Token", self->_token);
				http.setTimeout(5000);

				int code = http.GET();
				if (code == 200) {
					String body = http.getString();
					JsonDocument doc;
					DeserializationError err = deserializeJson(doc, body);
					if (!err && doc["command"] && doc["command"] != "null") {
						String cmd     = doc["command"].as<String>();
						String payload = doc["payload"] | "";

						if (cmd == "reboot") {
							self->warning("Reboot command received");
							http.end();
							ESP.restart();
						}
						if (cmd == "ping") {
							self->info("Pong!");
						}
						if (self->_commands.count(cmd)) {
							self->_commands[cmd](payload);
						}
					}
				}
				http.end();
			}
			vTaskDelay(pdMS_TO_TICKS(self->_commandInterval));
		}
	}
};