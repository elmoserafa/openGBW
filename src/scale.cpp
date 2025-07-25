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

// Taring message state variables
bool showingTaringMessage = false;
unsigned long taringMessageStartTime = 0;

// Manual grinding mode - grinder controlled directly by button
bool manualGrindMode = false;

double scaleFactor = 1409.88; // Standard scale factor, can be updated by calibration

bool tareScale()
{
    Serial.println("Taring scale (non-blocking)...");
    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (loadcell.wait_ready_timeout(1000)) {
            long offset = loadcell.read_average(10); // Average 10 readings for stability
            loadcell.set_offset(offset);
            lastTareAt = millis();
            scaleWeight = 0;
            // Reset Kalman filter after taring
            kalmanFilter = SimpleKalmanFilter(0.02, 0.02, 0.01);
            Serial.println("Scale tared successfully");
            return true;
        } else {
            Serial.print("Tare attempt ");
            Serial.print(attempt);
            Serial.println(": HX711 not ready, retrying...");
            delay(200);
        }
    }
    Serial.println("Tare failed: HX711 not ready after retries.");
    return false;
}

// Task to continuously update the scale readings
void updateScale(void *parameter) {
    float lastEstimate;
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS; // 10Hz = 100ms interval
    for (;;) {
        if (lastTareAt == 0) {
            Serial.println("retaring scale");
            Serial.println("current offset");
            Serial.println(offset);
            tareScale();
        }
        if (loadcell.wait_ready_timeout(300)) {
            if (scaleStatus == STATUS_GRINDING_IN_PROGRESS) {
                // Direct measurement, no averaging
                long raw = loadcell.read();
                double grams = (double)(raw - loadcell.get_offset()) / scaleFactor;
                scaleWeight = kalmanFilter.updateEstimate(grams);
                // Serial.printf("HX711 raw: %ld, offset: %ld, grams: %.2f, filtered: %.2f\n", raw, loadcell.get_offset(), grams, scaleWeight);
            } else {
                // Normal mode with averaging
                long raw = loadcell.read_average(5);
                double grams = (double)(raw - loadcell.get_offset()) / scaleFactor;
                scaleWeight = kalmanFilter.updateEstimate(grams);
                // Serial.printf("HX711 raw: %ld, offset: %ld, grams: %.2f, filtered: %.2f\n", raw, loadcell.get_offset(), grams, scaleWeight);
            }
            if (ABS(scaleWeight) < 3) {
                scaleWeight = 0;
            }
            scaleLastUpdatedAt = millis();
            weightHistory.push(scaleWeight);
            scaleReady = true;
        } else {
            Serial.println("HX711 not found.");
            scaleReady = false;
        }
        vTaskDelay(xDelay); // Wait 100ms before next read (10Hz)
    }
}

// Toggles the grinder on or off based on mode
void grinderToggle() {
    if (!scaleMode) {
        if (grindMode) {
            grinderActive = !grinderActive;
            digitalWrite(GRINDER_ACTIVE_PIN, grinderActive ? LOW : HIGH); // LOW = Relay ON, HIGH = Relay OFF
            Serial.print("Grinder toggled: ");
            Serial.println(grinderActive ? "ON" : "OFF");
        } else {
            Serial.println("Grinder ON (Impulse Mode)");
            digitalWrite(GRINDER_ACTIVE_PIN, LOW); // Relay ON
            delay(100);
            digitalWrite(GRINDER_ACTIVE_PIN, HIGH); // Relay OFF
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
                // Auto-tare is disabled except for startup (handled in updateScale)
                static bool grinderButtonPressed = false;
                static unsigned long grinderButtonPressedAt = 0;
                static bool manualGrinderActive = false;

                // Manual grind mode - direct control of grinder with button
                if (manualGrindMode) {
                    bool buttonCurrentlyPressed = (digitalRead(GRIND_BUTTON_PIN) == LOW);
                    if (buttonCurrentlyPressed && !manualGrinderActive) {
                        // Button just pressed - start grinder
                        manualGrinderActive = true;
                        digitalWrite(GRINDER_ACTIVE_PIN, LOW); // Relay ON
                        Serial.println("Manual grind: Grinder ON");
                        wakeScreen();
                    } else if (!buttonCurrentlyPressed && manualGrinderActive) {
                        // Button just released - stop grinder
                        manualGrinderActive = false;
                        digitalWrite(GRINDER_ACTIVE_PIN, HIGH); // Relay OFF
                        Serial.println("Manual grind: Grinder OFF");
                    }
                    break; // Skip automatic grinding logic when in manual mode
                }

                // Only allow button trigger if grindMode == true (automatic mode)
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
            if (scaleWeight < -10.0)
            { // Only fail if weight is significantly negative (cup removed)
                Serial.println("GRINDING FAILED: Significantly negative weight detected (cup removed).");
                grinderToggle(); // Ensure grinder is off
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            if (!scaleReady)
            {
                Serial.println("GRINDING FAILED: Scale not ready");
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
                if (scaleMode && startedGrindingAt == 0 && scaleWeight - cupWeightEmpty >= 0.1) {
                startedGrindingAt = millis();
                continue;
            }
                if (millis() - startedGrindingAt > MAX_GRINDING_TIME && !scaleMode) {
                Serial.println("GRINDING FAILED: Max grinding time exceeded");
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            if (millis() - startedGrindingAt > 5000 &&
                scaleWeight - weightHistory.firstValueOlderThan(millis() - 5000) < 1 &&
                    !scaleMode) {
                Serial.println("GRINDING FAILED: No weight increase after 2 seconds");
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
                if (weightHistory.minSince((int64_t)millis() - 200) < cupWeightEmpty - CUP_DETECTION_TOLERANCE && !scaleMode) {
                Serial.printf("GRINDING FAILED: Cup removed - min weight: %.2f, cup weight: %.2f, tolerance: %d\n", 
                             weightHistory.minSince((int64_t)millis() - 200), cupWeightEmpty, CUP_DETECTION_TOLERANCE);
                grinderToggle();
                scaleStatus = STATUS_GRINDING_FAILED;
                continue;
            }
            double currentOffset = offset;
                if (scaleMode) {
                currentOffset = 0;
            }
                double grindTarget;
                if (grindMode && !manualGrindMode) {
                    // Button-activated automatic grinding: ignore cup weight
                    grindTarget = setWeight + currentOffset;
                } else {
                    // Other modes: include cup weight
                    grindTarget = cupWeightEmpty + setWeight + currentOffset;
                }
                if (weightHistory.maxSince((int64_t)millis() - 200) >= grindTarget) {
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
            } else if (millis() - finishedGrindingAt > 2000 && newOffset && AUTO_OFFSET_ADJUSTMENT) {
                // Wait 2 seconds for all coffee to settle, then auto-adjust offset
                double targetTotalWeight;
                if (grindMode && !manualGrindMode) {
                    // Button-activated automatic grinding: ignore cup weight
                    targetTotalWeight = setWeight;
                } else {
                    // Other modes: include cup weight
                    targetTotalWeight = setWeight + cupWeightEmpty;
                }
                double actualWeight = currentWeight;
                double weightError = targetTotalWeight - actualWeight;

                if (ABS(weightError) > 0.3) { // Only adjust if error is significant (>0.3g)
                    double oldOffset = offset;
                    offset += weightError;
                    // Constrain offset to reasonable limits
                    if (offset > 5.0) offset = 5.0;
                    if (offset < -5.0) offset = -5.0;
                    // Serial output disabled for performance
                    shotCount++;
                    preferences.begin("scale", false);
                    preferences.putDouble("offset", offset);
                    preferences.putUInt("shotCount", shotCount);
                    preferences.end();
                } else {
                    shotCount++;
                    preferences.begin("scale", false);
                    preferences.putUInt("shotCount", shotCount);
                    preferences.end();
                }
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
    // Set HX711 to 10Hz (default debug mode)
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // default 10 Hz mode
    pinMode(GRINDER_ACTIVE_PIN, OUTPUT);
    pinMode(GRIND_BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(GRINDER_ACTIVE_PIN, HIGH); // Initialize HIGH = Relay OFF = Grinder stopped
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
    manualGrindMode = preferences.getBool("manualGrindMode", false);
    preferences.end();
  Serial.printf("→ scaleFactor = %.0f  |  offset = %.0f\n", scaleFactor, offset);
  Serial.printf("→ Manual Grind Mode: %s\n", manualGrindMode ? "ENABLED" : "DISABLED");
    // loadcell.set_scale(scaleFactor); // Not used in debug form
    // loadcell.set_offset(offset); // Not used in debug form

    xTaskCreatePinnedToCore(updateScale, "Scale", 10000, NULL, 0, &ScaleTask, 1);
    xTaskCreatePinnedToCore(scaleStatusLoop, "ScaleStatus", 10000, NULL, 0, &ScaleStatusTask, 1);
}
