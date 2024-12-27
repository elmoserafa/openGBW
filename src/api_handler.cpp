#include "api_handler.hpp"
#include "config.hpp"
#include <ArduinoJson.h> // For JSON parsing
#include <Preferences.h> // For handling persistent storage

extern double setWeight, offset;
extern bool scaleMode, grindMode;

void setupApiEndpoints(AsyncWebServer &server)
{
    // Serve HTML
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        extern const std::string html_page;
        request->send(200, "text/html", html_page.c_str()); });

    // Retrieve settings
    server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String json = "{";
        json += "\"setWeight\":" + String(setWeight) + ",";
        json += "\"offset\":" + String(offset) + ",";
        json += "\"scaleMode\":" + String(scaleMode) + ",";
        json += "\"grindMode\":" + String(grindMode);
        json += "}";
        request->send(200, "application/json", json); });

    // Update settings
    server.on("/updateSettings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
        // Convert the received data into a String
        String body = String((char*)data);
        
        // Parse the JSON body
        DynamicJsonDocument doc(256); // Ensure proper buffer size
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"error\":\"JSON parsing failed\"}");
            return;
        }

        // Update settings based on parsed JSON
        if (doc.containsKey("setWeight")) {
            setWeight = doc["setWeight"].as<double>();
            preferences.begin("scale", false);
            preferences.putDouble("setWeight", setWeight);
            preferences.end();
        }
        if (doc.containsKey("offset")) {
            offset = doc["offset"].as<double>();
            preferences.begin("scale", false);
            preferences.putDouble("offset", offset);
            preferences.end();
        }
        if (doc.containsKey("scaleMode")) {
            scaleMode = doc["scaleMode"].as<bool>();
            preferences.begin("scale", false);
            preferences.putBool("scaleMode", scaleMode);
            preferences.end();
        }
        if (doc.containsKey("grindMode")) {
            grindMode = doc["grindMode"].as<bool>();
            preferences.begin("scale", false);
            preferences.putBool("grindMode", grindMode);
            preferences.end();
        }

        // Send success response
        request->send(200, "application/json", "{\"message\":\"Settings updated successfully\"}"); });
}
