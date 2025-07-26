#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>

#include "display.hpp"
#include "scale.hpp"
#include "config.hpp"
#include "web_server.hpp"

// Definitions of global variables (memory allocated here)
Preferences preferences;             // Preferences object
HX711 loadcell;                      // HX711 load cell object
SimpleKalmanFilter kalmanFilter(0.02, 0.02, 0.01); // Kalman filter for weight smoothing

TaskHandle_t ScaleTask = nullptr;    // Initialize task handles to nullptr
TaskHandle_t ScaleStatusTask = nullptr;

volatile bool displayLock = false; 

void setup() {
    Serial.begin(115200);
    
    // Deactivate WiFi and Bluetooth on ESP32 startup
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
#ifdef ARDUINO_ARCH_ESP32
    btStop();
#endif
    // Setup other components
    setupDisplay();
    setupScale();
    // setupWebServer(); // Disabled
}

void loop() {
    delay(1000);
}
