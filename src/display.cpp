#include "display.hpp"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C screen(U8G2_R0);
TaskHandle_t DisplayTask;

// Time in milliseconds after which the display sleeps (10 seconds)
#define SLEEP_AFTER_MS 60 * 30000

// Function to center-align and print text to the screen
void CenterPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = screen.getStrWidth(str); // Calculate the text width
  screen.setCursor(128 / 2 - width / 2, y);    // Set the cursor position for center alignment
  screen.print(str);                           // Print the text
}

// Function to left-align and print text to the screen
void LeftPrintToScreen(char const *str, u8g2_uint_t y) {
  screen.setCursor(5, y);
  screen.print(str);
}

// Function to left-align and highlight text as active on the screen
void LeftPrintActiveToScreen(char const *str, u8g2_uint_t y) {
  screen.setDrawColor(1);            // Set drawing color to white
  screen.drawBox(3, y - 1, 122, 14); 
  screen.setDrawColor(0);          
  screen.setCursor(5, y);       
  screen.print(str);
  screen.setDrawColor(1);            // Reset drawing color to white
}

// Function to right-align and print text to the screen
void RightPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = screen.getStrWidth(str); // Calculate the text width
  screen.setCursor(123 - width, y);           // Set the cursor position for right alignment
  screen.print(str);                          // Print the text
}

// Function to display the menu with previous, current, and next items
void showMenu() {
  int prevIndex = (currentMenuItem - 1) % menuItemsCount; // Get the previous menu item index
  int nextIndex = (currentMenuItem + 1) % menuItemsCount; // Get the next menu item index

  // Handle negative index wrap-around
  prevIndex = prevIndex < 0 ? prevIndex + menuItemsCount : prevIndex;
  MenuItem prev = menuItems[prevIndex];   // Previous menu item
  MenuItem current = menuItems[currentMenuItem]; // Current menu item
  MenuItem next = menuItems[nextIndex];   // Next menu item

  char buf[3];
  screen.clearBuffer();                   // Clear the display buffer
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf);     // Set the font for the menu title
  CenterPrintToScreen("Menu", 0);      // Print "Menu" title
  screen.setFont(u8g2_font_7x13_tr);      // Set the font for the menu items
  LeftPrintToScreen(prev.menuName, 19); // Print the previous menu item
  LeftPrintActiveToScreen(current.menuName, 35); // Highlight the current menu item
  LeftPrintToScreen(next.menuName, 51); // Print the next menu item

  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the offset adjustment menu
void showOffsetMenu() {
  char buf[16];
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Adjust offset", 0); // Print the menu title
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the offset value
  snprintf(buf, sizeof(buf), "%3.2fg", offset); // Format the offset value
  CenterPrintToScreen(buf, 28); // Print the offset value
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the scale mode menu
void showScaleModeMenu() {
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Set Scale Mode", 0); // Print the menu title
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (scaleMode) {
    LeftPrintToScreen("GBW", 19); // Print inactive item
    LeftPrintActiveToScreen("Scale only", 35); // Highlight active item
  } else {
    LeftPrintActiveToScreen("GBW", 19); // Highlight active item
    LeftPrintToScreen("Scale only", 35); // Print inactive item
  }
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the grind mode menu
void showGrindModeMenu() {
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Set Grinder", 0); // Print the menu title
  CenterPrintToScreen("Start/Stop Mode", 19); // Print the subtitle
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (grindMode) {
    LeftPrintActiveToScreen("Continuous", 35); // Highlight active item
    LeftPrintToScreen("Impulse", 51); // Print inactive item
  } else {
    LeftPrintToScreen("Continuous", 35); // Print inactive item
    LeftPrintActiveToScreen("Impulse", 51); // Highlight active item
  }
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the cup weight adjustment menu
void showCupMenu() {
  char buf[16];
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Cup Weight", 0); // Print the menu title
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the instructions
  snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight); // Format the scale weight
  CenterPrintToScreen(buf, 19); // Print the scale weight
  LeftPrintToScreen("Place cup on scale", 35); // Print instructions
  LeftPrintToScreen("and press button", 51); // Print instructions
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the calibration menu
void showCalibrationMenu() {
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Calibration", 0); // Print the menu title
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the instructions
  CenterPrintToScreen("Place 100g weight", 19); // Print instructions
  CenterPrintToScreen("on scale and", 35); // Print instructions
  CenterPrintToScreen("press button", 51); // Print instructions
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the reset menu
void showResetMenu() {
  screen.clearBuffer();
  screen.setFontPosTop();
  screen.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Reset to defaults?", 0); // Print the menu title
  screen.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (greset) {
    LeftPrintActiveToScreen("Confirm", 19); // Highlight active item
    LeftPrintToScreen("Cancel", 35); // Print inactive item
  } else {
    LeftPrintToScreen("Confirm", 19); // Print inactive item
    LeftPrintActiveToScreen("Cancel", 35); // Highlight active item
  }
  screen.sendBuffer(); // Send the buffer to the display
}

// Function to display the appropriate menu or setting based on the current state
void showSetting() {
  if (currentSetting == 2) {
    showOffsetMenu();
  } else if (currentSetting == 0) {
    showCupMenu();
  } else if (currentSetting == 1) {
    showCalibrationMenu();
  } else if (currentSetting == 3) {
    showScaleModeMenu();
  } else if (currentSetting == 4) {
    showGrindModeMenu();
  } else if (currentSetting == 6) {
    showResetMenu();
  }
}

// Task to update the display with the current state
void updateDisplay(void *parameter) {
  char buf[64];
  char buf2[64];

  for (;;) {
    screen.clearBuffer(); // Clear the display buffer
    if (millis() - lastSignificantWeightChangeAt > SLEEP_AFTER_MS) {
      screen.sendBuffer(); // Send the buffer to the display to "sleep"
      delay(100);
      continue;
    }

    if (scaleLastUpdatedAt == 0) {
      screen.setFontPosTop();
      screen.drawStr(0, 20, "Initializing...");
    } else if (!scaleReady) {
      screen.setFontPosTop();
      screen.drawStr(0, 20, "SCALE ERROR");
    } else {
      if (scaleStatus == STATUS_GRINDING_IN_PROGRESS) {
        screen.setFontPosTop();
        screen.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Grinding...", 0);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_7x14B_tf);
        screen.setCursor(3, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
        screen.print(buf);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_unifont_t_symbols);
        screen.drawGlyph(64, 32, 0x2794);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_7x14B_tf);
        screen.setCursor(84, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", setWeight);
        screen.print(buf);

        screen.setFontPosBottom();
        screen.setFont(u8g2_font_7x13_tr);
        snprintf(buf, sizeof(buf), "%3.1fs", startedGrindingAt > 0 ? (double)(millis() - startedGrindingAt) / 1000 : 0);
        CenterPrintToScreen(buf, 64);
      } else if (scaleStatus == STATUS_EMPTY) {
        screen.setFontPosTop();
        screen.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Weight:", 0);

        screen.setFont(u8g2_font_7x14B_tf);
        screen.setFontPosCenter();
        screen.setCursor(0, 28);
        snprintf(buf, sizeof(buf), "%3.1fg", abs(scaleWeight));
        CenterPrintToScreen(buf, 32);

        screen.setFont(u8g2_font_7x13_tf);
        screen.setFontPosCenter();
        screen.setCursor(5, 50);
        snprintf(buf2, sizeof(buf2), "Set: %3.1fg", setWeight);
        LeftPrintToScreen(buf2, 50);
      } else if (scaleStatus == STATUS_GRINDING_FAILED) {
        screen.setFontPosTop();
        screen.setFont(u8g2_font_7x14B_tf);
        CenterPrintToScreen("Grinding failed", 0);

        screen.setFontPosTop();
        screen.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Press the balance", 32);
        CenterPrintToScreen("to reset", 42);
      } else if (scaleStatus == STATUS_GRINDING_FINISHED) {
        screen.setFontPosTop();
        screen.setFont(u8g2_font_7x13_tr);
        screen.setCursor(0, 0);
        CenterPrintToScreen("Grinding finished", 0);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_7x14B_tf);
        screen.setCursor(3, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
        screen.print(buf);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_unifont_t_symbols);
        screen.drawGlyph(64, 32, 0x2794);

        screen.setFontPosCenter();
        screen.setFont(u8g2_font_7x14B_tf);
        screen.setCursor(84, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", setWeight);
        screen.print(buf);

        screen.setFontPosBottom();
        screen.setFont(u8g2_font_7x13_tr);
        screen.setCursor(64, 64);
        snprintf(buf, sizeof(buf), "%3.1fs", (double)(finishedGrindingAt - startedGrindingAt) / 1000);
        CenterPrintToScreen(buf, 64);
      } else if (scaleStatus == STATUS_IN_MENU) {
        showMenu();
      } else if (scaleStatus == STATUS_IN_SUBMENU) {
        showSetting();
      }
    }
    screen.sendBuffer(); // Send the buffer to the display
  }
}

// Function to initialize the display and start the display update task
void setupDisplay() {
  screen.begin(); // Initialize the display
  screen.setFont(u8g2_font_7x13_tr); // Set the default font
  screen.setFontPosTop();
  screen.drawStr(0, 20, "Hello"); // Display a welcome message

  // Create a task to update the display
  xTaskCreatePinnedToCore(
      updateDisplay, /* Function to implement the task */
      "Display",    /* Name of the task */
      10000,         /* Stack size in words */
      NULL,          /* Task input parameter */
      0,             /* Priority of the task */
      &DisplayTask,  /* Task handle */
      1);            /* Core where the task should run */
}
