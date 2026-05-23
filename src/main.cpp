#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../config/config.h"

WebServer server(80);

void publishToServer(const char* event_type, const char* source_device,
                     const char* details) {
    StaticJsonDocument<512> doc;
    doc["timestamp"]     = millis();
    doc["type"]          = "HoneypotIoT";
    doc["source"]        = "hardware";
    doc["device"]        = DEVICE_ID;
    doc["event_type"]    = event_type;
    doc["source_device"] = source_device;
    doc["details"]       = details;
    doc["src_ip"]        = WiFi.localIP().toString();

    char buffer[512];
    serializeJson(doc, buffer);

    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(buffer);
    http.end();

    Serial.printf("[HTTP->SERVER] %s rc=%d\n", buffer, code);
}

void handleEvent() {
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json",
            "{\"error\":\"Method not allowed\"}");
        return;
    }

    String body = server.arg("plain");
    Serial.printf("[EVENT] %s\n", body.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json",
            "{\"error\":\"Invalid JSON\"}");
        return;
    }

    const char* device     = doc["device"]     | "unknown";
    const char* event_type = doc["event_type"] | "unknown";
    const char* details    = doc["details"]    | "";

    publishToServer(event_type, device, details);
    server.send(200, "application/json", "{\"status\":\"received\"}");
}

void handleInfo() {
    publishToServer("INFO_REQUEST", DEVICE_ID,
        WiFi.localIP().toString().c_str());
    server.send(200, "application/json",
        "{\"model\":\"MicroPLC-X100\","
        "\"firmware\":\"v2.0.0\","
        "\"manufacturer\":\"IndustrialSoft\","
        "\"protocol\":\"HTTP\","
        "\"connected_devices\":[\"camera\"]}");
}

void handleStatus() {
    publishToServer("STATUS_REQUEST", DEVICE_ID,
        WiFi.localIP().toString().c_str());
    server.send(200, "application/json",
        "{\"status\":\"running\","
        "\"uptime\":3600,"
        "\"connected_devices\":1,"
        "\"alarms\":0}");
}

void handleCommand() {
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json",
            "{\"error\":\"Method not allowed\"}");
        return;
    }
    String body = server.arg("plain");
    publishToServer("COMMAND_ATTEMPT", DEVICE_ID, body.c_str());
    server.send(200, "application/json", "{\"status\":\"executed\"}");
}

void handleFirmware() {
    String body = server.arg("plain");
    publishToServer("OTA_ATTEMPT", DEVICE_ID, body.c_str());
    server.send(200, "application/json",
        "{\"status\":\"update_scheduled\"}");
}

void handleNotFound() {
    publishToServer("UNKNOWN_ENDPOINT", DEVICE_ID,
        server.uri().c_str());
    server.send(404, "application/json", "{\"error\":\"Not found\"}");
}

void connectWifi() {
    Serial.printf("\nConectando a %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi conectado. IP: %s\n",
        WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(115200);
    connectWifi();

    publishToServer("DEVICE_ONLINE", DEVICE_ID,
        WiFi.localIP().toString().c_str());

    server.on("/event",           HTTP_POST, handleEvent);
    server.on("/info",            HTTP_GET,  handleInfo);
    server.on("/status",          HTTP_GET,  handleStatus);
    server.on("/command",         HTTP_POST, handleCommand);
    server.on("/firmware/update", HTTP_POST, handleFirmware);
    server.onNotFound(handleNotFound);
    server.begin();

    Serial.println("PLC-Gateway HTTP iniciado en puerto 80");
}

void loop() {
    server.handleClient();
}