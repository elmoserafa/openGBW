#include "api_handler.hpp"
#include "config.hpp"

extern double setWeight, offset;
extern bool scaleMode, grindMode;

void setupApiEndpoints(AsyncWebServer& server) {
    // Serve HTML
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        extern const std::string html_page;
        request->send(200, "text/html", html_page.c_str());
    });

    // Retrieve settings
    server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"setWeight\":" + String(setWeight) + ",";
        json += "\"offset\":" + String(offset) + ",";
        json += "\"scaleMode\":" + String(scaleMode) + ",";
        json += "\"grindMode\":" + String(grindMode);
        json += "}";
        request->send(200, "application/json", json);
    });

    // Update settings
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
}
