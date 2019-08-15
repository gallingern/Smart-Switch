#include <blynk.h>
#include "smart-switch.h"
#include "ZeroCrossDimmer.h"

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
char auth[] = "4025c2c641f74330a7475890f4f42674";
int temp_f = 100;
int sunset_hour = 17; // 5pm
int sunset_minute = 0;
int wake_hour = 6;
int wake_minute = 30;
int sleep_hour = 22; // 10pm
int sleep_minute = 0;
bool switch1_light = false;
bool switch2_heat = false;
BlynkTimer blynk_timer;


// get info from other photon
void myTempHandler(const char *event, const char *data) {
  temp_f = atoi(data);
}
void myHourHandler(const char *event, const char *data) {
  sunset_hour = atoi(data);
}
void myMinuteHandler(const char *event, const char *data) {
  sunset_minute = atoi(data);
}


// Get switch input from blynk app
BLYNK_WRITE(V1) {
  if (param.asInt() == 0) { switch1_light = false; }
  else { switch1_light = true; }
}
BLYNK_WRITE(V2) {
  if (param.asInt() == 0) { switch2_heat = false; }
  else { switch2_heat = true; }
}
BLYNK_WRITE(V3) {
  TimeInputParam t(param);
  wake_hour = t.getStartHour();
  wake_minute = t.getStartMinute();
  sleep_hour = t.getStopHour();
  sleep_minute = t.getStopMinute();
}


void updateBlynk() {
  // send temp to blynk app
  Blynk.virtualWrite(V0, temp_f);
  // update button states on blynk app
  Blynk.virtualWrite(V1, switch1_light);
  Blynk.virtualWrite(V2, switch2_heat);

  // Update time input
  // timezone
  char tz[] = "US/Pacific";
  //seconds from the start of a day. 0 - min, 86399 - max
  int startAt = ((wake_hour*60) + wake_minute) * 60;
  //seconds from the start of a day. 0 - min, 86399 - max
  int stopAt = ((sleep_hour*60) + sleep_minute) * 60;
  Blynk.virtualWrite(V3, startAt, stopAt, tz);
}


void setup() {
  ZeroCrossDimmer_init();
  // Relay setup
  pinMode(SWITCH1_PIN, OUTPUT);
  pinMode(SWITCH2_PIN, OUTPUT);

  Particle.variable("temp_f", &temp_f, INT);
  Particle.variable("sunset_hr", &sunset_hour, INT);
  Particle.variable("sunset_min", &sunset_minute, INT);
  Particle.variable("wake_hour", &wake_hour, INT);
  Particle.variable("wake_minute", &wake_minute, INT);
  Particle.variable("sleep_hour", &sleep_hour, INT);
  Particle.variable("sleep_minute", &sleep_minute, INT);

  Particle.subscribe("temp", myTempHandler, MY_DEVICES);
  Particle.subscribe("hour", myHourHandler, MY_DEVICES);
  Particle.subscribe("minute", myMinuteHandler, MY_DEVICES);

  Blynk.begin(auth);

  // Setup a function to be called every second
  blynk_timer.setInterval(1000L, updateBlynk);
}


// Relays are active LOW
void triggerSwitch1() {
  if (switch1_light) {
    digitalWrite(SWITCH1_PIN, LOW);
  }
  else {
    digitalWrite(SWITCH1_PIN, HIGH);
  }
}
void triggerSwitch2() {
  if (switch2_heat) {
    digitalWrite(SWITCH2_PIN, LOW);
  }
  else {
    digitalWrite(SWITCH2_PIN, HIGH);
  }
}


void checkLight() {
  int today_wake_hour = wake_hour;
  int today_wake_minute = wake_minute;
  int today_sleep_hour = sleep_hour;
  int today_sleep_minute = sleep_minute;

  // Weekend wake at 7:30am
  if (isWeekend()) {
    today_wake_hour = 7;
    today_wake_minute = 30;
  }
  // Weekend night sleep at 10:30pm
  if (isWeekendNight()) {
    today_sleep_hour = 22;
    today_sleep_minute = 30;
  }

  // ******************** Morning On ********************
  if ((Time.hour() == today_wake_hour) &&
      (Time.minute() == today_wake_minute) &&
      (!ZeroCrossDimmer_isDimming())) {

    ZeroCrossDimmer_startDimOn();
    switch1_light = true;

    // heat
    int threshold_f = 60; // degrees f
    if (temp_f < threshold_f) {
      switch2_heat = true;
    }
  }

  int morning_off_hour = 8;
  int morning_off_minute = 30;
  // ******************** Morning Off ********************
  if ((Time.hour() == morning_off_hour) && (Time.minute() == morning_off_minute)) {
    switch1_light = false;
    switch2_heat = false;
  }

  // ******************** Evening On ********************
  // 30 min before sunset trigger (also a hack)
  if (sunset_minute < 30) {
    if ((Time.hour() == (sunset_hour - 1)) && (Time.minute() == (sunset_minute + 30))) {
      switch1_light = true;
    }
  }
  else {
    if ((Time.hour() == sunset_hour) && (Time.minute() == (sunset_minute - 30))) {
      switch1_light = true;
    }
  }

  // ******************** Evening Off ********************
  if ((Time.hour() == today_sleep_hour) && (Time.minute() == today_sleep_minute)) {
      switch1_light = false;
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  checkLight();
  triggerSwitch1();
  triggerSwitch2();

  Blynk.run();
  blynk_timer.run();
}
