#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>


#include "display.hpp"
#include "scale.hpp"


void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  
  setupDisplay();
  setupScale();

  Serial.println();
  Serial.println("******************************************************");
}

void loop() {
  delay(1000);
}
