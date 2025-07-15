#include "config.hpp"
#include "rotary.hpp"
#include "scale.hpp"
#include "display.hpp"

// Variables for scale functionality
double scaleWeight = 0;       // Current weight measured by the scale
double setWeight = 0;         // Target weight set by the user
double setCupWeight = 0;      // Weight of the cup set by the user
double offset = 0;            // Offset for stopping grinding prior to reaching set weight
bool scaleMode = false;       // Indicates if the scale is used in timer mode
bool grindMode = false;       // Grinder mode: impulse (false) or continuous (true)
bool grinderActive = false;   // Grinder state (on/off)
unsigned int shotCount;

// Buffer for storing recent weight history
MathBuffer<double, 100> weightHistory;

// Timing and status variables
unsigned long scaleLastUpdatedAt = 0;  // Timestamp of the last scale update
unsigned long lastSignificantWeightChangeAt = 0; // Timestamp of the last significant weight change
unsigned long lastTareAt = 0; // Timestamp of the last tare operation
bool scaleReady = false;      // Indicates if the scale is ready to measure
int scaleStatus = STATUS_EMPTY; // Current status of the scale
double cupWeightEmpty = 0;    // Measured weight of the empty cup
unsigned long startedGrindingAt = 0;  // Timestamp of when grinding started
unsigned long finishedGrindingAt = 0; // Timestamp of when grinding finished
bool greset = false;          // Flag for reset operation
bool newOffset = false;       // Indicates if a new offset value is pending

bool useButtonToGrind = DEFAULT_GRIND_TRIGGER_MODE;

void tareScale()
{
    Serial.println("Taring scale...");
    loadcell.tare(TARE_MEASURES); // Perform the tare operation
    delay(500);                   // Allow the load cell to stabilize
    lastTareAt = millis();        // Update the timestamp
    scaleWeight = 0;              // Reset the displayed weight
    Serial.println("Scale tared successfully");
}

// Task to continuously update the scale readings
void updateScale(void *parameter) {
    float lastEstimate;
    for (;;) {
        if (lastTareAt == 0) {
            Serial.println("retaring scale");
            Serial.println("current offset");
            Serial.println(offset);
            tareScale();
        }
        if (loadcell.wait_ready_timeout(300)) {
            lastEstimate = kalmanFilter.updateEstimate(loadcell.get_units(5));
            scaleWeight = lastEstimate;
            // Serial.printf("Scale reading: %.2f g\n", scaleWeight);
            if (ABS(scaleWeight) < 3)
            {
                scaleWeight = 0;
            }
            scaleLastUpdatedAt = millis();
            weightHistory.push(scaleWeight);
            scaleReady = true;
        } else {
            Serial.println("HX711 not found.");
            scaleReady = false;
        }
    }
}

// Toggles the grinder on or off based on mode
// GPIO HIGH = NPN transistor ON = Pulls grinder 5V signal to ground = Grinder starts
// GPIO LOW = NPN transistor OFF = Grinder 5V signal stays high = Grinder stops
void grinderToggle() {
    if (!scaleMode) {
        if (grindMode) {
            grinderActive = !grinderActive;
            digitalWrite(GRINDER_ACTIVE_PIN, grinderActive);
            Serial.print("Grinder toggled: ");
            Serial.println(grinderActive ? "ON" : "OFF");
        } else {
            Serial.println("Grinder ON (Impulse Mode)");
            digitalWrite(GRINDER_ACTIVE_PIN, 1); // Pull 5V signal to ground via NPN
            delay(100);
            digitalWrite(GRINDER_ACTIVE_PIN, 0); // Release 5V signal (back to HIGH)
            Serial.println("Grinder OFF");
        }
    }
}

// Task to manage the status of the scale
void scaleStatusLoop(void *p) {
    for (;;) {
        double tenSecAvg = weightHistory.averageSince((int64_t)millis() - 10000);
        if (ABS(tenSecAvg - scaleWeight) > SIGNIFICANT_WEIGHT_CHANGE) {
            lastSignificantWeightChangeAt = millis();
        }

        switch (scaleStatus) {
            case STATUS_EMPTY: {
                if (millis() - lastTareAt > TARE_MIN_INTERVAL && ABS(tenSecAvg) > 0.2 && tenSecAvg < 3 && scaleWeight < 3) {
                    lastTareAt = 0; // Retare if conditions are met
                }
            
                static bool grinderButtonPressed = false;
                static unsigned long grinderButtonPressedAt = 0;
            
                // Only allow button trigger if grindMode == true
                if (grindMode && digitalRead(GRIND_BUTTON_PIN) == LOW && !grinderButtonPressed) {
                    grinderButtonPressed = true;
                    grinderButtonPressedAt = millis();
                    wakeScreen(); // wake screen immediately
                    Serial.println("Grinder button pressed, screen waking...");
                }
            
                if (grindMode && grinderButtonPressed && millis() - grinderButtonPressedAt >= 600) {
                    grinderButtonPressed = false; // reset flag
                    cupWeightEmpty = scaleWeight;
                    scaleStatus = STATUS_GRINDING_IN_PROGRESS;
                    if (!scaleMode) {
                        newOffset = true;
                        startedGrindingAt = millis();
                    }
                    grinderToggle();
                    Serial.println("Grinding started after delay.");
                    continue;
                }
            
                // Only allow cup trigger if grindMode == false
                if (!grindMode &&
                    ABS(weightHistory.minSince(millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE &&
                    ABS(weightHistory.maxSince(millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE) {
                    
                    cupWeightEmpty = weightHistory.averageSince(millis() - 500);
                    scaleStatus = STATUS_GRINDING_IN_PROGRESS;
                    if (!scaleMode) {
                        newOffset = true;
                        startedGrindingAt = millis();
                    }
                    grinderToggle();
                    Serial.println("Grinding started from cup detection.");
                    continue;
                }
            
                break;
            }            
        case STATUS_GRINDING_IN_PROGRESS:
        {
            if (scaleWeight <= 0)
            { // Avoid restarting grinding with zero or negative weight
                Serial.println("Negative or zero weight detected. Skipping grinding.");
                grinderToggle(); // Ensure grinder is off
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            if (!scaleReady)
            {
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
            }
                if (scaleMode && startedGrindingAt == 0 && scaleWeight - cupWeightEmpty >= 0.1) {
                startedGrindingAt = millis();
                continue;
            }
                if (millis() - startedGrindingAt > MAX_GRINDING_TIME && !scaleMode) {
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            if (millis() - startedGrindingAt > 2000 &&
                scaleWeight - weightHistory.firstValueOlderThan(millis() - 2000) < 1 &&
                    !scaleMode) {
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
                if (weightHistory.minSince((int64_t)millis() - 200) < cupWeightEmpty - CUP_DETECTION_TOLERANCE && !scaleMode) {
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            double currentOffset = offset;
                if (scaleMode) {
                currentOffset = 0;
            }
                if (weightHistory.maxSince((int64_t)millis() - 200) >= cupWeightEmpty + setWeight + currentOffset) {
                finishedGrindingAt = millis();
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FINISHED;
                continue;
            }
            break;
        }
        case STATUS_GRINDING_FINISHED:
        {
            static unsigned long grindingFinishedAt = 0;

            // Record the time when grinding finished if not already recorded
            if (grindingFinishedAt == 0)
            {
                grindingFinishedAt = millis();
                Serial.print("Grinder was on for: ");
                Serial.print(grindingFinishedAt);
                Serial.println(" seconds");
            }

            double currentWeight = weightHistory.averageSince((int64_t)millis() - 500);
                if (scaleWeight < 5) {
                startedGrindingAt = 0;
                grindingFinishedAt = 0; // Reset the timestamp
                scaleWeight = 0;
                scaleStatus = STATUS_EMPTY;
                continue;
                } else if (currentWeight != setWeight + cupWeightEmpty && millis() - finishedGrindingAt > 1500 && newOffset) {
                offset += setWeight + cupWeightEmpty - currentWeight;
                    if (ABS(offset) >= setWeight) {
                    offset = COFFEE_DOSE_OFFSET;
                }
                shotCount++;
                preferences.begin("scale", false);
                preferences.putDouble("offset", offset);
                preferences.putUInt("shotCount", shotCount);
                preferences.end();
                newOffset = false;
            }

            // Timeout to transition back to the main menu after grinding finishes
            if (millis() - grindingFinishedAt > 5000)
            { // 5-second delay after grinding finishes
                if (scaleWeight >= 3)
                { // If weight is still on the scale, wait for cup removal
                    Serial.println("Waiting for cup to be removed...");
                }
                else
                {
                    startedGrindingAt = 0;
                    grindingFinishedAt = 0; // Reset the timestamp
                    scaleStatus = STATUS_EMPTY;
                    Serial.println("Grinding finished. Transitioning to main menu.");
                }
            }
            break;
        }
        case STATUS_GRINDING_FAILED:
        {
            if (scaleWeight >= GRINDING_FAILED_WEIGHT_TO_RESET)
            {
                scaleStatus = STATUS_EMPTY;
                continue;
            }
            break;
        }
        }
        rotary_loop();
        delay(50);
    }
}

// Alternative ISR function that calls the library ISR
void IRAM_ATTR encoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

// Initializes the scale hardware and settings
void setupScale() {
    Serial.println("Initializing rotary encoder...");
    Serial.print("Encoder A pin: ");
    Serial.println(ROTARY_ENCODER_A_PIN);
    Serial.print("Encoder B pin: ");
    Serial.println(ROTARY_ENCODER_B_PIN);
    Serial.print("Encoder Button pin: ");
    Serial.println(ROTARY_ENCODER_BUTTON_PIN);
    
    // Set pin modes first with pullups
    pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println("Pin modes set with pullups");
    
    // Test initial pin states
    Serial.println("Testing encoder pins directly:");
    Serial.print("Pin A state: ");
    Serial.println(digitalRead(ROTARY_ENCODER_A_PIN));
    Serial.print("Pin B state: ");
    Serial.println(digitalRead(ROTARY_ENCODER_B_PIN));
    
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    
    // Enable circuitry for encoder
    rotaryEncoder.enable();
    
    // Try manual interrupt attachment as fallback
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_A_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_B_PIN), encoderISR, CHANGE);
    
    Serial.println("Interrupts attached manually as fallback");
    
    // Set boundaries and acceleration - make it more responsive
    rotaryEncoder.setBoundaries(-10000, 10000, true);
    rotaryEncoder.setAcceleration(0); // Disable acceleration for more predictable response
    
    // Test encoder by reading initial value
    int initialValue = rotaryEncoder.readEncoder();
    Serial.print("Initial encoder value: ");
    Serial.println(initialValue);
    
    Serial.println("Rotary encoder initialized successfully.");
    
    Serial.println("Initializing load cell...");
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    pinMode(GRINDER_ACTIVE_PIN, OUTPUT);
    pinMode(GRIND_BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(GRINDER_ACTIVE_PIN, 0); // Initialize LOW = NPN OFF = Grinder 5V signal HIGH = Grinder stopped
    Serial.println("Load cell and pins initialized.");

    preferences.begin("scale", false);
    
    double scaleFactor = preferences.getDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
    if (scaleFactor <= 0 || std::isnan(scaleFactor)) {
    scaleFactor = LOADCELL_SCALE_FACTOR;
    preferences.putDouble("calibration", scaleFactor);
    Serial.println("Invalid scale factor detected. Resetting to default.");
    }
    setWeight = preferences.getDouble("setWeight", (double)COFFEE_DOSE_WEIGHT);
    offset = preferences.getDouble("offset", (double)COFFEE_DOSE_OFFSET);
    setCupWeight = preferences.getDouble("cup", (double)CUP_WEIGHT);
    scaleMode = preferences.getBool("scaleMode", false);
    grindMode = preferences.getBool("grindMode", false);
    shotCount = preferences.getUInt("shotCount", 0);
    sleepTime = preferences.getInt("sleepTime", SLEEP_AFTER_MS); // Default to SLEEP_AFTER_MS if not set
    useButtonToGrind = preferences.getBool("grindTrigger", DEFAULT_GRIND_TRIGGER_MODE);
    preferences.end();
  Serial.printf("→ scaleFactor = %.0f  |  offset = %.0f\n", scaleFactor, offset);
    loadcell.set_scale(scaleFactor);
    loadcell.set_offset(offset);

    xTaskCreatePinnedToCore(updateScale, "Scale", 10000, NULL, 0, &ScaleTask, 1);
    xTaskCreatePinnedToCore(scaleStatusLoop, "ScaleStatus", 10000, NULL, 0, &ScaleStatusTask, 1);
}
