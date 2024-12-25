#pragma once

#include "config.hpp"

void rotary_onButtonClick();
void rotary_loop();
void readEncoderISR();

extern AiEsp32RotaryEncoder rotaryEncoder;