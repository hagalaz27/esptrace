#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class ESPLogger {
public:

    // ── Конструктор ───────────────────────────────────────────
    ESPLogger(const char* token, const char* serverUrl = "https://esplogger.com/api/log.php") {
        _token     = token;
        _serverUrl = serverUrl;
        _enabled   = true;
        _minLevel  = LOG_DEBUG;
    }

    // ── Уровни логирования ────────────────────────────────────
    enum Level { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARNING = 2, LOG_ERROR = 3 };

    // ── Настройки ─────────────────────────────────────────────

    // Включить/выключить логгер
    void enable(bool state) { _enabled = state; }

    // Минимальный уровень — логи ниже этого уровня игнорируются
    void setMinLevel(Level level) { _minLevel = level; }

    // Кастомный URL сервера
    void setServerUrl(const char* url) { _serverUrl = url; }

    // ── Основные методы логирования ───────────────────────────

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

    // Универсальный метод
    bool log(const String& message, const String& level = "INFO") {
        return send(message, level);
    }

    // ── Удобные методы для ESP32 ──────────────────────────────

    // Логировать причину перезагрузки
    bool logResetReason() {
        esp_reset_reason_t reason = esp_reset_reason();
        String msg = "Reset reason: " + resetReasonToString(reason);
        return send(msg, "WARNING");
    }

    // Логировать системную информацию
    bool logSystemInfo() {
        StaticJsonDocument<256> doc;
        doc["free_heap"]    = ESP.getFreeHeap();
        doc["chip_model"]   = ESP.getChipModel();
        doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
        doc["flash_size"]   = ESP.getFlashChipSize();
        doc["uptime_sec"]   = millis() / 1000;
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // Логировать данные WiFi
    bool logWifiInfo() {
        if (WiFi.status() != WL_CONNECTED) {
            return send("WiFi not connected", "WARNING");
        }
        StaticJsonDocument<256> doc;
        doc["ip"]      = WiFi.localIP().toString();
        doc["mac"]     = WiFi.macAddress();
        doc["ssid"]    = WiFi.SSID();
        doc["rssi"]    = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // Логировать произвольный JSON объект
    bool logJson(JsonDocument& doc, const String& level = "INFO") {
        String json;
        serializeJson(doc, json);
        return send(json, level);
    }

    // Логировать значение сенсора
    bool logSensor(const String& name, float value, const String& unit = "") {
        StaticJsonDocument<128> doc;
        doc[name] = value;
        if (unit.length() > 0) doc["unit"] = unit;
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // Логировать несколько сенсоров сразу
    bool logSensors(const char* names[], const float values[], const int count) {
        StaticJsonDocument<512> doc;
        for (int i = 0; i < count; i++) {
            doc[names[i]] = values[i];
        }
        String json;
        serializeJson(doc, json);
        return send(json, "INFO");
    }

    // ── Получить последнюю ошибку ─────────────────────────────
    String getLastError() { return _lastError; }

    // ── Статус последней отправки ─────────────────────────────
    bool lastSendOk() { return _lastSendOk; }

private:
    const char* _token;
    const char* _serverUrl;
    bool        _enabled;
    Level       _minLevel;
    String      _lastError;
    bool        _lastSendOk = false;

    // ── Отправка на сервер ────────────────────────────────────
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

        // Собираем JSON
        StaticJsonDocument<2048> doc;
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
};