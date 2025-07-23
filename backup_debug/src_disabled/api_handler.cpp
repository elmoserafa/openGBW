#include "api_handler.hpp"
#include "config.hpp"

extern Preferences preferences;

void setupApiEndpoints(AsyncWebServer& server) {
    // Serve Wi-Fi configuration page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        extern const std::string html_page;
        request->send(200, "text/html", html_page.c_str());
    });

    // Handle Wi-Fi settings submission
    server.on("/updateSettings", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
            String ssid = request->getParam("ssid", true)->value();
            String password = request->getParam("password", true)->value();

            preferences.begin("wifi", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();

            request->send(200, "text/html", "<h1>Wi-Fi Saved. Restarting...</h1>");
            delay(3000);
            ESP.restart(); // Restart ESP32 to apply new Wi-Fi settings
        } else {
            request->send(400, "text/plain", "Missing Wi-Fi credentials");
        }
    });
}
