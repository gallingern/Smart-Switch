#include "Switch.h"

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
bool switch1_light = false;
bool switch2_heat = false;

// Relays are active LOW
void Switch_init() {
  pinMode(SWITCH1_PIN, OUTPUT);
  pinMode(SWITCH2_PIN, OUTPUT);
}

void Switch_lightOn() {
  digitalWrite(SWITCH1_PIN, LOW);
  switch1_light = true;
}

void Switch_lightOff() {
  digitalWrite(SWITCH1_PIN, HIGH);
  switch1_light = false;
}

bool Switch_getLightState() {
  return switch1_light;
}

void Switch_heatOn() {
  digitalWrite(SWITCH2_PIN, LOW);
  switch2_heat = true;
}

void Switch_heatOff() {
  digitalWrite(SWITCH2_PIN, HIGH);
  switch2_heat = false;
}

bool Switch_getHeatState() {
  return switch2_heat;
}
