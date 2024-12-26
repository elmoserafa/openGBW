#pragma once

#include "config.hpp"

void rotary_onButtonClick();
void rotary_loop();
void readEncoderISR();
void exitToMenu();

extern AiEsp32RotaryEncoder rotaryEncoder;