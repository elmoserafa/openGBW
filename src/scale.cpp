#include "scale.hpp"
#include <MathBuffer.h>
#include <AiEsp32RotaryEncoder.h>
#include <Preferences.h>

// HX711 load cell for measuring weight
HX711 loadcell;

// Kalman filter for smoothing weight measurements
SimpleKalmanFilter kalmanFilter(0.02, 0.02, 0.01);

// Preferences for saving and retrieving settings
Preferences preferences;

// Rotary encoder for user input
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
    ROTARY_ENCODER_A_PIN, 
    ROTARY_ENCODER_B_PIN, 
    ROTARY_ENCODER_BUTTON_PIN, 
    ROTARY_ENCODER_VCC_PIN, 
    ROTARY_ENCODER_STEPS);

// Macro to calculate the absolute value
#define ABS(a) (((a) > 0) ? (a) : ((a) * -1))

// Task handles for FreeRTOS tasks
TaskHandle_t ScaleTask;
TaskHandle_t ScaleStatusTask;

// Variables for scale functionality
double scaleWeight = 0;       // Current weight measured by the scale
double setWeight = 0;         // Target weight set by the user
double setCupWeight = 0;      // Weight of the cup set by the user
double offset = 0;            // Offset for stopping grinding prior to reaching set weight
bool scaleMode = false;       // Indicates if the scale is used in timer mode
bool grindMode = false;       // Grinder mode: impulse (false) or continuous (true)
bool grinderActive = false;   // Grinder state (on/off)

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
int encoderDir = 1;           // Direction of the rotary encoder
bool greset = false;          // Flag for reset operation
bool newOffset = false;       // Indicates if a new offset value is pending

// Menu items for user interface
int currentMenuItem = 0;      // Index of the current menu item
int currentSetting;           // Index of the current setting being adjusted
int encoderValue = 0;         // Current value of the rotary encoder
int menuItemsCount = 7;       // Total number of menu items
MenuItem menuItems[7] = {
    {1, false, "Cup weight", 1, &setCupWeight},
    {2, false, "Calibrate", 0},
    {3, false, "Offset", 0.1, &offset},
    {4, false, "Scale Mode", 0},
    {5, false, "Grinding Mode", 0},
    {6, false, "Exit", 0},
    {7, false, "Reset", 0}}; // Menu items for settings and calibration

// Handles button clicks on the rotary encoder
void rotary_onButtonClick() {
    static unsigned long lastTimePressed = 0; // Timestamp of the last button press
    if (millis() - lastTimePressed < 500) {
        // Debounce multiple button presses
        return;
    }
    if (scaleStatus == STATUS_EMPTY) {
        // Enter the menu when the scale is empty
        scaleStatus = STATUS_IN_MENU;
        currentMenuItem = 0;
        rotaryEncoder.setAcceleration(0);
    } else if (scaleStatus == STATUS_IN_MENU) {
        // Handle menu navigation
        switch (currentMenuItem) {
            case 5:
                scaleStatus = STATUS_EMPTY;
                rotaryEncoder.setAcceleration(100);
                Serial.println("Exited Menu");
                break;
            case 2: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 2;
                Serial.println("Offset Menu");
                break;
            }
            case 0: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 0;
                Serial.println("Cup Menu");
                break;
            }
            case 1: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 1;
                Serial.println("Calibration Menu");
                break;
            }
            case 3: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 3;
                Serial.println("Scale Mode Menu");
                break;
            }
            case 4: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 4;
                Serial.println("Grind Mode Menu");
                break;
            }
            case 6: {
                scaleStatus = STATUS_IN_SUBMENU;
                currentSetting = 6;
                greset = false;
                Serial.println("Reset Menu");
                break;
            }
        }
    } else if (scaleStatus == STATUS_IN_SUBMENU) {
        // Handle submenu actions based on the current setting
        switch (currentSetting) {
            case 2: {
                preferences.begin("scale", false);
                preferences.putDouble("offset", offset);
                preferences.end();
                scaleStatus = STATUS_IN_MENU;
                currentSetting = -1;
                break;
            }
            case 0: {
                if (scaleWeight > 30) { // Prevent accidental setting with no cup
                    setCupWeight = scaleWeight;
                    Serial.println(setCupWeight);
                    preferences.begin("scale", false);
                    preferences.putDouble("cup", setCupWeight);
                    preferences.end();
                    scaleStatus = STATUS_IN_MENU;
                    currentSetting = -1;
                }
                break;
            }
            case 1: {
                preferences.begin("scale", false);
                double newCalibrationValue = preferences.getDouble("calibration", 1.0) * (scaleWeight / 100);
                Serial.println(newCalibrationValue);
                preferences.putDouble("calibration", newCalibrationValue);
                preferences.end();
                loadcell.set_scale(newCalibrationValue);
                scaleStatus = STATUS_IN_MENU;
                currentSetting = -1;
                break;
            }
            case 3: {
                preferences.begin("scale", false);
                preferences.putBool("scaleMode", scaleMode);
                preferences.end();
                scaleStatus = STATUS_IN_MENU;
                currentSetting = -1;
                break;
            }
            case 4: {
                preferences.begin("scale", false);
                preferences.putBool("grindMode", grindMode);
                preferences.end();
                scaleStatus = STATUS_IN_MENU;
                currentSetting = -1;
                break;
            }
            case 6: {
                if (greset) {
                    preferences.begin("scale", false);
                    preferences.putDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
                    setWeight = (double)COFFEE_DOSE_WEIGHT;
                    preferences.putDouble("setWeight", (double)COFFEE_DOSE_WEIGHT);
                    offset = (double)COFFEE_DOSE_OFFSET;
                    preferences.putDouble("offset", (double)COFFEE_DOSE_OFFSET);
                    setCupWeight = (double)CUP_WEIGHT;
                    preferences.putDouble("cup", (double)CUP_WEIGHT);
                    scaleMode = false;
                    preferences.putBool("scaleMode", false);
                    grindMode = false;
                    preferences.putBool("grindMode", false);
                    loadcell.set_scale((double)LOADCELL_SCALE_FACTOR);
                    preferences.end();
                }
                scaleStatus = STATUS_IN_MENU;
                currentSetting = -1;
                break;
            }
        }
    }
}

// Handles rotary encoder input for menu navigation and adjustments
void rotary_loop() {
    if (rotaryEncoder.encoderChanged()) {
        switch (scaleStatus) {
            case STATUS_EMPTY: {
                // Adjust weight when in scale mode
                int newValue = rotaryEncoder.readEncoder();
                setWeight += ((float)newValue - (float)encoderValue) / 10 * encoderDir;
                encoderValue = newValue;
                preferences.begin("scale", false);
                preferences.putDouble("setWeight", setWeight);
                preferences.end();
                break;
            }
            case STATUS_IN_MENU: {
                // Navigate through menu items
                int newValue = rotaryEncoder.readEncoder();
                currentMenuItem = (currentMenuItem + (newValue - encoderValue) * -encoderDir) % menuItemsCount;
                currentMenuItem = currentMenuItem < 0 ? menuItemsCount + currentMenuItem : currentMenuItem;
                encoderValue = newValue;
                Serial.println(currentMenuItem);
                break;
            }
            case STATUS_IN_SUBMENU: {
                int newValue = rotaryEncoder.readEncoder();
                if (currentSetting == 2) { // Offset menu
                    offset += ((float)newValue - (float)encoderValue) * encoderDir / 100;
                    encoderValue = newValue;
                    if (abs(offset) >= setWeight) {
                        offset = setWeight; // Prevent nonsensical offsets
                    }
                } else if (currentSetting == 3) {
                    scaleMode = !scaleMode;
                } else if (currentSetting == 4) {
                    grindMode = !grindMode;
                } else if (currentSetting == 6) {
                    greset = !greset;
                }
                break;
            }
        }
    }
    if (rotaryEncoder.isEncoderButtonClicked()) {
        rotary_onButtonClick();
    }
}

// ISR for reading encoder changes
void readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

// Tares the scale (sets the current weight to zero)
void tareScale() {
    Serial.println("Taring scale");
    loadcell.tare(TARE_MEASURES);
    lastTareAt = millis();
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
void grinderToggle() {
    if (!scaleMode) {
        if (grindMode) {
            grinderActive = !grinderActive;
            digitalWrite(GRINDER_ACTIVE_PIN, grinderActive);
        } else {
            digitalWrite(GRINDER_ACTIVE_PIN, 1);
            delay(100);
            digitalWrite(GRINDER_ACTIVE_PIN, 0);
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
                if (ABS(weightHistory.minSince((int64_t)millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE &&
                    ABS(weightHistory.maxSince((int64_t)millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE) {
                    cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
                    scaleStatus = STATUS_GRINDING_IN_PROGRESS;
                    if (!scaleMode) {
                        newOffset = true;
                        startedGrindingAt = millis();
                    }
                    grinderToggle();
                    continue;
                }
                break;
            }
            case STATUS_GRINDING_IN_PROGRESS: {
                if (!scaleReady) {
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
            case STATUS_GRINDING_FINISHED: {
                double currentWeight = weightHistory.averageSince((int64_t)millis() - 500);
                if (scaleWeight < 5) {
                    startedGrindingAt = 0;
                    scaleStatus = STATUS_EMPTY;
                    continue;
                } else if (currentWeight != setWeight + cupWeightEmpty && millis() - finishedGrindingAt > 1500 && newOffset) {
                    offset += setWeight + cupWeightEmpty - currentWeight;
                    if (ABS(offset) >= setWeight) {
                        offset = COFFEE_DOSE_OFFSET;
                    }
                    preferences.begin("scale", false);
                    preferences.putDouble("offset", offset);
                    preferences.end();
                    newOffset = false;
                }
                break;
            }
            case STATUS_GRINDING_FAILED: {
                if (scaleWeight >= GRINDING_FAILED_WEIGHT_TO_RESET) {
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

// Initializes the scale hardware and settings
void setupScale() {
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(-10000, 10000, true);
    rotaryEncoder.setAcceleration(100);

    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    pinMode(GRINDER_ACTIVE_PIN, OUTPUT);
    digitalWrite(GRINDER_ACTIVE_PIN, 0);

    preferences.begin("scale", false);
    double scaleFactor = preferences.getDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
    setWeight = preferences.getDouble("setWeight", (double)COFFEE_DOSE_WEIGHT);
    offset = preferences.getDouble("offset", (double)COFFEE_DOSE_OFFSET);
    setCupWeight = preferences.getDouble("cup", (double)CUP_WEIGHT);
    scaleMode = preferences.getBool("scaleMode", false);
    grindMode = preferences.getBool("grindMode", false);
    preferences.end();

    loadcell.set_scale(scaleFactor);

    xTaskCreatePinnedToCore(updateScale, "Scale", 10000, NULL, 0, &ScaleTask, 1);
    xTaskCreatePinnedToCore(scaleStatusLoop, "ScaleStatus", 10000, NULL, 0, &ScaleStatusTask, 1);
}
