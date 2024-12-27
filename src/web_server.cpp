#include "web_server.hpp"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config.hpp"
#include "api_handler.hpp"

// Wi-Fi credentials
const char *ssid = "ssid";
const char *password = "pass";

// Web server instance
AsyncWebServer server(80);

void setupWebServer() {
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup API endpoints
    setupApiEndpoints(server);

    // Start the server
    server.begin();
}
