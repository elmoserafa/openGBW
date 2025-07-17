#pragma once

#include "config.hpp"
#include <SPI.h>
#include <U8g2lib.h>

void setupDisplay();
void showCupWeightSetScreen(double cupWeight);
void showInfoMenu();
void showModeMenu();
void showConfigMenu();
void wakeScreen();
void showIpAddress();
void showTaringMessage();
void showModeChangeMessage(const char* mode, const char* status);