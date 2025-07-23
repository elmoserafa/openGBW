#include <Arduino.h>
#include "HX711.h"

// Pin definitions (from full firmware)
#define LOADCELL_DOUT_PIN  19
#define LOADCELL_SCK_PIN   18

HX711 loadcell;


// Global variables (declare only once)
unsigned long sampleCount = 0;
unsigned long lastRatePrint = 0;
unsigned long lastSampleCount = 0;
float scaleFactor = 1409.88f; // Standard scale factor
bool calibrated = false;

void setup() {
    Serial.begin(115200);
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    Serial.println("Place the scale on a stable surface and ensure it is unloaded.");
    delay(2000);
    Serial.println("Taring... Please wait.");
    loadcell.tare();
    Serial.println("Tare complete. Place a 100g weight on the scale and type 'c' in the serial monitor to calibrate.");
}


void calibrateScale() {
    Serial.println("\n--- Calibration Start ---");
    Serial.println("Remove all weight from the scale. Press ENTER in the serial monitor to tare.");
    // Wait for ENTER
    while (!Serial.available()) {
        delay(10);
    }
    while (Serial.available()) Serial.read(); // clear buffer
    loadcell.tare();
    Serial.println("Tare complete. Place your calibration weight on the scale and enter its value in grams, then press ENTER.");
    // Wait for user to enter weight value
    String input = "";
    while (input.length() == 0) {
        if (Serial.available()) {
            input = Serial.readStringUntil('\n');
            input.trim();
        }
        delay(10);
    }
    float knownWeight = input.toFloat();
    if (knownWeight <= 0.0f) {
        Serial.println("Invalid weight entered. Calibration aborted.");
        return;
    }
    Serial.print("Calibrating with weight: ");
    Serial.print(knownWeight, 2);
    Serial.println(" g");
    long reading = loadcell.get_value(10); // average 10 readings with weight
    if (reading == 0) {
        Serial.println("Calibration failed: reading is zero.");
        return;
    }
    scaleFactor = (float)reading / knownWeight;
    calibrated = true;
    Serial.print("Calibration complete. Scale factor: ");
    Serial.println(scaleFactor, 4);
    Serial.println("--- Calibration End ---\n");
}

void loop() {
    static unsigned long lastSampleTime = 0;
    unsigned long now = millis();

    // Check for serial input to trigger calibration or print scale factor
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'c' || cmd == 'C') {
            calibrateScale();
        } else if (cmd == 's' || cmd == 'S') {
            Serial.print("Current scale factor: ");
            Serial.println(scaleFactor, 4);
        }
    }

    if (now - lastSampleTime >= 100) { // 100 ms interval = 10 Hz
        lastSampleTime = now;
        if (loadcell.is_ready()) {
            sampleCount++;
            long raw = loadcell.get_value(1);
            Serial.print("Sample: ");
            Serial.print(sampleCount);
            Serial.print(" | is_ready: YES");
            Serial.print(" | Raw ADC: ");
            Serial.print(raw);
            if (calibrated && scaleFactor != 0.0f) {
                float grams = (float)raw / scaleFactor;
                Serial.print(" | Grams: ");
                Serial.print(grams, 2);
            }
            Serial.println();
        } else {
            Serial.print("Sample: ");
            Serial.print(sampleCount);
            Serial.println(" | is_ready: NO | Raw ADC: HX711 not found.");
        }
    }

    if (now - lastRatePrint >= 1000) {
        unsigned long samplesInLastSec = sampleCount - lastSampleCount;
        Serial.print("Measured sample rate: ");
        Serial.print(samplesInLastSec);
        Serial.println(" Hz");
        lastSampleCount = sampleCount;
        lastRatePrint = now;
    }
}
