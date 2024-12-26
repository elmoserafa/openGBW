#include "config.hpp"
#include "rotary.hpp"
#include "display.hpp"

// Rotary encoder for user input
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
    ROTARY_ENCODER_A_PIN,
    ROTARY_ENCODER_B_PIN,
    ROTARY_ENCODER_BUTTON_PIN,
    ROTARY_ENCODER_VCC_PIN,
    ROTARY_ENCODER_STEPS);

// Vars
int encoderDir = 1;   // Direction of the rotary encoder
int encoderValue = 0; // Current value of the rotary encoder

// Incase you can't set something you can exit
void exitToMenu()
{
    if (scaleStatus == STATUS_IN_SUBMENU || scaleStatus == STATUS_INFO_MENU)
    {
        scaleStatus = STATUS_IN_MENU;
        currentSetting = -1;
        Serial.println("Exiting to main menu");
    }
    else if (scaleStatus == STATUS_IN_MENU)
    {
        scaleStatus = STATUS_EMPTY;
        Serial.println("Exiting to empty state");
    }
}

// Handles button clicks on the rotary encoder
void rotary_onButtonClick()
{
    static unsigned long lastTimePressed = 0; // Timestamp of the last button press
    if (millis() - lastTimePressed < 500)
    {
        // Debounce multiple button presses
        return;
    }
    if (scaleStatus == STATUS_EMPTY)
    {
        // Enter the menu when the scale is empty
        scaleStatus = STATUS_IN_MENU;
        currentMenuItem = 0;
        rotaryEncoder.setAcceleration(0);
    }
    else if (scaleStatus == STATUS_IN_MENU)
    {
        switch (currentMenuItem)
        {
        case 0: // Cup Weight Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 0;
            Serial.println("Cup Menu");
            break;
        case 1: // Calibration Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 1;
            Serial.println("Calibration Menu");
            break;
        case 2: // Offset Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 2;
            Serial.println("Offset Menu");
            break;
        case 3: // Scale Mode Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 3;
            Serial.println("Scale Mode Menu");
            break;
        case 4: // Grinding Mode Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 4;
            Serial.println("Grind Mode Menu");
            break;
        case 5: // Info Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 5;
            Serial.println("Info Menu");
            break;
        case 6: // Sleep Timer Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 8;
            Serial.println("Sleep Timer Menu");
            break;
        case 7: // Exit
            scaleStatus = STATUS_EMPTY;
            rotaryEncoder.setAcceleration(100);
            Serial.println("Exited Menu");
            break;
        case 8: // Reset Menu
            scaleStatus = STATUS_IN_SUBMENU;
            currentSetting = 6;
            Serial.println("Reset Menu");
            break;
        }
    }

    else if (scaleStatus == STATUS_IN_SUBMENU)
    {
        // Handle submenu actions based on the current setting
        switch (currentSetting)
        {
        case 0:
        { // Cup Weight Menu
            if (scaleWeight > 30)
            { // Prevent accidental setting with no cup
                setCupWeight = scaleWeight;
                Serial.println(setCupWeight);

                preferences.begin("scale", false);
                preferences.putDouble("cup", setCupWeight);
                preferences.end();

                // Lock the display and show the confirmation
                displayLock = true;
                showCupWeightSetScreen(setCupWeight);
                displayLock = false;

                // Return to the menu
                exitToMenu();
            }
            else
            {
                Serial.println("Failsafe: Exiting cup weight menu due to zero weight");
                exitToMenu();
            }
            break;
        }
        case 1: // calibration menu
        {
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
        case 2:
        { // Offset Menu
            preferences.begin("scale", false);
            preferences.putDouble("offset", offset);
            preferences.end();
            scaleStatus = STATUS_IN_MENU;
            currentSetting = -1;
            break;
        }
        case 3: // scale menu
        {
            preferences.begin("scale", false);
            preferences.putBool("scaleMode", scaleMode);
            preferences.end();
            scaleStatus = STATUS_IN_MENU;
            currentSetting = -1;
            break;
        }
        case 4: // grinding menu
        {
            preferences.begin("scale", false);
            preferences.putBool("grindMode", grindMode);
            preferences.end();
            scaleStatus = STATUS_IN_MENU;
            currentSetting = -1;
            break;
        }
        case 5:
        { // Info Menu
            // Lock the display and show the Info Menu
            displayLock = true;
            showInfoMenu();
            delay(3000); // Keep the Info Menu visible for 3 seconds
            displayLock = false;

            // After showing the info, return to the main menu
            exitToMenu();
            break;
        }
        case 6: // reset
        {
            if (greset)
            {
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
        case 8: // Sleep Timer Menu
        {
            preferences.begin("scale", false);
            preferences.putInt("sleepTime", sleepTime); // Save sleep time
            preferences.end();
            scaleStatus = STATUS_IN_MENU;
            currentSetting = -1;
            break;
        }
        }
    }
}

// Handles rotary encoder input for menu navigation and adjustments
void rotary_loop()
{
    if (rotaryEncoder.encoderChanged())
    {
        // Wake the screen if it's asleep
        if (millis() - lastSignificantWeightChangeAt > sleepTime)
        {
            Serial.println("Screen waking due to rotary movement...");
            wakeScreen();
        }
        switch (scaleStatus)
        {
        case STATUS_EMPTY:
        {
            if (screenJustWoke)
            {
                // Skip modifying the set weight if the screen just woke up
                screenJustWoke = false; // Reset the flag
                break;
            }
            // Adjust weight when in scale mode
            int newValue = rotaryEncoder.readEncoder();
            setWeight += ((float)newValue - (float)encoderValue) / 10 * encoderDir;
            encoderValue = newValue;
            preferences.begin("scale", false);
            preferences.putDouble("setWeight", setWeight);
            preferences.end();
            break;
        }
        case STATUS_IN_MENU:
        {
            // Navigate through menu items
            int newValue = rotaryEncoder.readEncoder();
            currentMenuItem = (currentMenuItem + (newValue - encoderValue) * -encoderDir) % menuItemsCount;
            currentMenuItem = currentMenuItem < 0 ? menuItemsCount + currentMenuItem : currentMenuItem;
            encoderValue = newValue;
            Serial.println(currentMenuItem);
            break;
        }
        case STATUS_IN_SUBMENU:
        {
            int newValue = rotaryEncoder.readEncoder();
            if (currentSetting == 2)
            { // Offset menu
                offset += ((float)newValue - (float)encoderValue) * encoderDir / 100;
                encoderValue = newValue;
                if (abs(offset) >= setWeight)
                {
                    offset = setWeight; // Prevent nonsensical offsets
                }
            }
            else if (currentSetting == 3)
            {
                scaleMode = !scaleMode;
            }
            else if (currentSetting == 4)
            {
                grindMode = !grindMode;
            }
            else if (currentSetting == 6)
            {
                greset = !greset;
            }
            else if (currentSetting == 8)
            {                                                  // Sleep Timer menu
                sleepTime += (newValue - encoderValue) * 1000; // Adjust by seconds
                if (sleepTime < 5000)
                    sleepTime = 5000; // Minimum sleep time: 5 seconds
                if (sleepTime > 600000)
                    sleepTime = 600000; // Maximum sleep time: 10 minutes
                encoderValue = newValue;
            }
            break;
        }
        }
    }
    if (rotaryEncoder.isEncoderButtonClicked())
    {
        rotary_onButtonClick();
    }
}

// ISR for reading encoder changes
void readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}