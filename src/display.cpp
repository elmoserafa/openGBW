#include "display.hpp"

// Initialize the display driver for SSD1306 128x64 OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Task handle for managing the display update task
TaskHandle_t DisplayTask;

// Time in milliseconds after which the display sleeps (10 seconds)
#define SLEEP_AFTER_MS 60 * 30000

// Function to center-align and print text to the screen
void CenterPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = u8g2.getStrWidth(str); // Calculate the text width
  u8g2.setCursor(128 / 2 - width / 2, y);    // Set the cursor position for center alignment
  u8g2.print(str);                           // Print the text
}

// Function to left-align and print text to the screen
void LeftPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2.setCursor(5, y); // Set the cursor position for left alignment
  u8g2.print(str);      // Print the text
}

// Function to left-align and highlight text as active on the screen
void LeftPrintActiveToScreen(char const *str, u8g2_uint_t y) {
  u8g2.setDrawColor(1);            // Set drawing color to white
  u8g2.drawBox(3, y - 1, 122, 14); // Draw a box to highlight the active text
  u8g2.setDrawColor(0);            // Set text color to black
  u8g2.setCursor(5, y);            // Set the cursor position for the active text
  u8g2.print(str);                 // Print the text
  u8g2.setDrawColor(1);            // Reset drawing color to white
}

// Function to right-align and print text to the screen
void RightPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = u8g2.getStrWidth(str); // Calculate the text width
  u8g2.setCursor(123 - width, y);           // Set the cursor position for right alignment
  u8g2.print(str);                          // Print the text
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
  u8g2.clearBuffer();                   // Clear the display buffer
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf);     // Set the font for the menu title
  CenterPrintToScreen("Menu", 0);      // Print "Menu" title
  u8g2.setFont(u8g2_font_7x13_tr);      // Set the font for the menu items
  LeftPrintToScreen(prev.menuName, 19); // Print the previous menu item
  LeftPrintActiveToScreen(current.menuName, 35); // Highlight the current menu item
  LeftPrintToScreen(next.menuName, 51); // Print the next menu item

  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the offset adjustment menu
void showOffsetMenu() {
  char buf[16];
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Adjust offset", 0); // Print the menu title
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the offset value
  snprintf(buf, sizeof(buf), "%3.2fg", offset); // Format the offset value
  CenterPrintToScreen(buf, 28); // Print the offset value
  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the scale mode menu
void showScaleModeMenu() {
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Set Scale Mode", 0); // Print the menu title
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (scaleMode) {
    LeftPrintToScreen("GBW", 19); // Print inactive item
    LeftPrintActiveToScreen("Scale only", 35); // Highlight active item
  } else {
    LeftPrintActiveToScreen("GBW", 19); // Highlight active item
    LeftPrintToScreen("Scale only", 35); // Print inactive item
  }
  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the grind mode menu
void showGrindModeMenu() {
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Set Grinder", 0); // Print the menu title
  CenterPrintToScreen("Start/Stop Mode", 19); // Print the subtitle
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (grindMode) {
    LeftPrintActiveToScreen("Continuous", 35); // Highlight active item
    LeftPrintToScreen("Impulse", 51); // Print inactive item
  } else {
    LeftPrintToScreen("Continuous", 35); // Print inactive item
    LeftPrintActiveToScreen("Impulse", 51); // Highlight active item
  }
  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the cup weight adjustment menu
void showCupMenu() {
  char buf[16];
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Cup Weight", 0); // Print the menu title
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the instructions
  snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight); // Format the scale weight
  CenterPrintToScreen(buf, 19); // Print the scale weight
  LeftPrintToScreen("Place cup on scale", 35); // Print instructions
  LeftPrintToScreen("and press button", 51); // Print instructions
  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the calibration menu
void showCalibrationMenu() {
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Calibration", 0); // Print the menu title
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the instructions
  CenterPrintToScreen("Place 100g weight", 19); // Print instructions
  CenterPrintToScreen("on scale and", 35); // Print instructions
  CenterPrintToScreen("press button", 51); // Print instructions
  u8g2.sendBuffer(); // Send the buffer to the display
}

// Function to display the reset menu
void showResetMenu() {
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_7x14B_tf); // Set the font for the menu title
  CenterPrintToScreen("Reset to defaults?", 0); // Print the menu title
  u8g2.setFont(u8g2_font_7x13_tr); // Set the font for the menu items
  if (greset) {
    LeftPrintActiveToScreen("Confirm", 19); // Highlight active item
    LeftPrintToScreen("Cancel", 35); // Print inactive item
  } else {
    LeftPrintToScreen("Confirm", 19); // Print inactive item
    LeftPrintActiveToScreen("Cancel", 35); // Highlight active item
  }
  u8g2.sendBuffer(); // Send the buffer to the display
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
    u8g2.clearBuffer(); // Clear the display buffer
    if (millis() - lastSignificantWeightChangeAt > SLEEP_AFTER_MS) {
      u8g2.sendBuffer(); // Send the buffer to the display to "sleep"
      delay(100);
      continue;
    }

    if (scaleLastUpdatedAt == 0) {
      u8g2.setFontPosTop();
      u8g2.drawStr(0, 20, "Initializing...");
    } else if (!scaleReady) {
      u8g2.setFontPosTop();
      u8g2.drawStr(0, 20, "SCALE ERROR");
    } else {
      if (scaleStatus == STATUS_GRINDING_IN_PROGRESS) {
        u8g2.setFontPosTop();
        u8g2.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Grinding...", 0);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_7x14B_tf);
        u8g2.setCursor(3, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
        u8g2.print(buf);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_unifont_t_symbols);
        u8g2.drawGlyph(64, 32, 0x2794);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_7x14B_tf);
        u8g2.setCursor(84, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", setWeight);
        u8g2.print(buf);

        u8g2.setFontPosBottom();
        u8g2.setFont(u8g2_font_7x13_tr);
        snprintf(buf, sizeof(buf), "%3.1fs", startedGrindingAt > 0 ? (double)(millis() - startedGrindingAt) / 1000 : 0);
        CenterPrintToScreen(buf, 64);
      } else if (scaleStatus == STATUS_EMPTY) {
        u8g2.setFontPosTop();
        u8g2.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Weight:", 0);

        u8g2.setFont(u8g2_font_7x14B_tf);
        u8g2.setFontPosCenter();
        u8g2.setCursor(0, 28);
        snprintf(buf, sizeof(buf), "%3.1fg", abs(scaleWeight));
        CenterPrintToScreen(buf, 32);

        u8g2.setFont(u8g2_font_7x13_tf);
        u8g2.setFontPosCenter();
        u8g2.setCursor(5, 50);
        snprintf(buf2, sizeof(buf2), "Set: %3.1fg", setWeight);
        LeftPrintToScreen(buf2, 50);
      } else if (scaleStatus == STATUS_GRINDING_FAILED) {
        u8g2.setFontPosTop();
        u8g2.setFont(u8g2_font_7x14B_tf);
        CenterPrintToScreen("Grinding failed", 0);

        u8g2.setFontPosTop();
        u8g2.setFont(u8g2_font_7x13_tr);
        CenterPrintToScreen("Press the balance", 32);
        CenterPrintToScreen("to reset", 42);
      } else if (scaleStatus == STATUS_GRINDING_FINISHED) {
        u8g2.setFontPosTop();
        u8g2.setFont(u8g2_font_7x13_tr);
        u8g2.setCursor(0, 0);
        CenterPrintToScreen("Grinding finished", 0);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_7x14B_tf);
        u8g2.setCursor(3, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
        u8g2.print(buf);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_unifont_t_symbols);
        u8g2.drawGlyph(64, 32, 0x2794);

        u8g2.setFontPosCenter();
        u8g2.setFont(u8g2_font_7x14B_tf);
        u8g2.setCursor(84, 32);
        snprintf(buf, sizeof(buf), "%3.1fg", setWeight);
        u8g2.print(buf);

        u8g2.setFontPosBottom();
        u8g2.setFont(u8g2_font_7x13_tr);
        u8g2.setCursor(64, 64);
        snprintf(buf, sizeof(buf), "%3.1fs", (double)(finishedGrindingAt - startedGrindingAt) / 1000);
        CenterPrintToScreen(buf, 64);
      } else if (scaleStatus == STATUS_IN_MENU) {
        showMenu();
      } else if (scaleStatus == STATUS_IN_SUBMENU) {
        showSetting();
      }
    }
    u8g2.sendBuffer(); // Send the buffer to the display
  }
}

// Function to initialize the display and start the display update task
void setupDisplay() {
  u8g2.begin(); // Initialize the display
  u8g2.setFont(u8g2_font_7x13_tr); // Set the default font
  u8g2.setFontPosTop();
  u8g2.drawStr(0, 20, "Hello"); // Display a welcome message

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
