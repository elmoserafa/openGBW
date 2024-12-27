#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>


#include "display.hpp"
#include "scale.hpp"
#include "config.hpp"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config.hpp"

// Wi-Fi credentials
const char *ssid = "mujjuanna";
const char *password = "Bismillah9200@mujju@nn@";

// Web server instance
AsyncWebServer server(80);

void setupWebServer() {
    // Serve a simple HTML page for testing
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", "<h1>Scale Configuration</h1>");
    });

    // Endpoint to retrieve settings
    server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"setWeight\":" + String(setWeight) + ",";
        json += "\"offset\":" + String(offset) + ",";
        json += "\"scaleMode\":" + String(scaleMode) + ",";
        json += "\"grindMode\":" + String(grindMode);
        json += "}";
        request->send(200, "application/json", json);
    });

    // Endpoint to update settings
    server.on("/updateSettings", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("setWeight", true)) {
            setWeight = request->getParam("setWeight", true)->value().toDouble();
            preferences.begin("scale", false);
            preferences.putDouble("setWeight", setWeight);
            preferences.end();
        }
        if (request->hasParam("offset", true)) {
            offset = request->getParam("offset", true)->value().toDouble();
            preferences.begin("scale", false);
            preferences.putDouble("offset", offset);
            preferences.end();
        }
        if (request->hasParam("scaleMode", true)) {
            scaleMode = request->getParam("scaleMode", true)->value() == "true";
            preferences.begin("scale", false);
            preferences.putBool("scaleMode", scaleMode);
            preferences.end();
        }
        if (request->hasParam("grindMode", true)) {
            grindMode = request->getParam("grindMode", true)->value() == "true";
            preferences.begin("scale", false);
            preferences.putBool("grindMode", grindMode);
            preferences.end();
        }
        request->send(200, "text/plain", "Settings updated");
    });

    // Start the server
    server.begin();
}

void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup other components
    setupDisplay();
    setupScale();
    setupWebServer();
}

void loop() {
    delay(1000);
}
