#include <blynk.h>
#include "SparkIntervalTimer.h"
#include "elapsedMillis.h"

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
const int PST_OFFSET  = -8;
const int PDT_OFFSET  = -7;

char auth[] = "4025c2c641f74330a7475890f4f42674";
int temp_f = 100;
int sunsetHour = 17; // 5pm
int sunsetMinute = 0;
int wakeHour = 8;
int wakeMinute = 0;
int sleepHour = 22; // 10pm
int sleepMinute = 0;
bool switch1_light = false;
bool switch2_heat = false;
BlynkTimer timer;

// Dimmer setup
int freqStep = 65;
volatile int counter = 0;         // Variable to use as a counter volatile as it is in an interrupt
volatile boolean zero_cross = 0;  // Boolean to store a "switch" to tell us if we have crossed zero
int AC_PIN = D3;                  // Output to Opto Triac
int PIN_ZERO_CROSS = D2;          // Interrrupt pin
// Min/Max depends on your circuit
// This is for my bedside LED lights
int DIM_MIN = 0; // 90 == on
int DIM_MAX = 128;
int dim = DIM_MIN;                // Dimming level (0-128)  0 = on, 128 = 0ff
IntervalTimer timer_dimmer;

static int MINUTE = 60000; // minute in milliseconds
bool start_dimmer = false;
elapsedMillis timeElapsed;


// Check for AC zero cross
void zero_cross_detect() {
  zero_cross = true;         // set the boolean to true to tell our dimming function that a zero cross has occured
  counter = 0;
  digitalWrite(AC_PIN, LOW); // turn off TRIAC (and AC)
}


// Turn on the TRIAC at the appropriate time
void dim_check() {
  if(zero_cross == true) {
    if(counter >= dim) {
      digitalWrite(AC_PIN, HIGH); // turn on light
      counter = 0;                // reset time step counter
      zero_cross = false;         //reset zero cross detection
    }
    else {
      counter++; // increment time step counter
    }
  }
}


// get info from other photon
void myTempHandler(const char *event, const char *data) {
  temp_f = atoi(data);
}
void myHourHandler(const char *event, const char *data) {
  sunsetHour = atoi(data);
}
void myMinuteHandler(const char *event, const char *data) {
  sunsetMinute = atoi(data);
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
  wakeHour = t.getStartHour();
  wakeMinute = t.getStartMinute();
  sleepHour = t.getStopHour();
  sleepMinute = t.getStopMinute();
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
  int startAt = ((wakeHour*60) + wakeMinute) * 60;
  //seconds from the start of a day. 0 - min, 86399 - max
  int stopAt = ((sleepHour*60) + sleepMinute) * 60;
  Blynk.virtualWrite(V3, startAt, stopAt, tz);
}


void setup() {
  // Relay setup
  pinMode(SWITCH1_PIN, OUTPUT);
  pinMode(SWITCH2_PIN, OUTPUT);

  // Zero Cross setup
  pinMode(PIN_ZERO_CROSS, INPUT);
  pinMode(AC_PIN, OUTPUT);
  timer_dimmer.begin(dim_check, freqStep, uSec, TIMER5);
  // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  attachInterrupt(PIN_ZERO_CROSS, zero_cross_detect, RISING);

  Particle.variable("temp_f", &temp_f, INT);
  Particle.variable("sunset_hr", &sunsetHour, INT);
  Particle.variable("sunset_min", &sunsetMinute, INT);
  Particle.variable("wake_hour", &wakeHour, INT);
  Particle.variable("wake_minute", &wakeMinute, INT);
  Particle.variable("sleep_hour", &sleepHour, INT);
  Particle.variable("sleep_minute", &sleepMinute, INT);
  Particle.variable("dimmer", &dim, INT);

  Particle.subscribe("temp", myTempHandler, MY_DEVICES);
  Particle.subscribe("hour", myHourHandler, MY_DEVICES);
  Particle.subscribe("minute", myMinuteHandler, MY_DEVICES);

  Blynk.begin(auth);

  // Setup a function to be called every second
  timer.setInterval(1000L, updateBlynk);
}


// Warning, contains a hack
bool isDaylightSavingsTime() {
  int dayOfMonth = Time.day();
  int month = Time.month();
  int dayOfWeek = Time.weekday() - 1; // make Sunday 0 .. Saturday 6
  const int MARCH = 3;
  const int APRIL = 4;
  const int OCTOBER = 10;
  const int NOVEMBER = 11;
  bool isDaylightSavings = false;

  // April to October is DST
  if ((month >= APRIL) && (month <= OCTOBER)) {
    isDaylightSavings = true;
  }

  // March after second Sunday is DST
  if (month == MARCH) {
    // voodoo magic
    if ((dayOfMonth - dayOfWeek) > 8) {
      isDaylightSavings = true;
    }
  }

  // November before first Sunday is DST
  if (month == NOVEMBER) {
    // voodoo magic
    if (!((dayOfMonth - dayOfWeek) > 1)) {
      isDaylightSavings = true;
    }
  }

  return isDaylightSavings;
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


// True if Saturday or Sunday
bool isWeekend() {
  int dayOfWeek = Time.weekday();
  return ((dayOfWeek == 7) || (dayOfWeek == 1));
}
// True if Friday or Saturday
bool isWeekendNight() {
  int dayOfWeek = Time.weekday();
  return ((dayOfWeek == 6) || (dayOfWeek == 7));
}


void checkLight() {
  int todayWakeHour = wakeHour;
  int todayWakeMinute = wakeMinute;
  int todaySleepHour = sleepHour;
  int todaySleepMinute = sleepMinute;

  // Weekend wake at 8am
  if (isWeekend()) {
    todayWakeHour = 8;
    todayWakeMinute = 0;
  }
  // Weekend night sleep at 11pm
  if (isWeekendNight()) {
    todaySleepHour = 23;
    todaySleepMinute = 0;
  }

  // ******************** Morning On ********************
  if ((Time.hour() == todayWakeHour) && (Time.minute() == todayWakeMinute)) {
    // light
    switch1_light = true;

    // dimmer
    dim = DIM_MAX;
    timeElapsed = 0;
    start_dimmer = true;

    // heat
    int threshold_f = 60; // degrees f
    if (temp_f < threshold_f) {
      switch2_heat = true;
    }
  }

  int morningOffHour = 8;
  int morningOffMinute = 30;
  // ******************** Morning Off ********************
  if ((Time.hour() == morningOffHour) && (Time.minute() == morningOffMinute)) {
    switch1_light = false;
    switch2_heat = false;
  }

  // ******************** Evening On ********************
  // 30 min before sunset trigger (also a hack)
  if (sunsetMinute < 30) {
    if ((Time.hour() == (sunsetHour - 1)) && (Time.minute() == (sunsetMinute + 30))) {
      switch1_light = true;
    }
  }
  else {
    if ((Time.hour() == sunsetHour) && (Time.minute() == (sunsetMinute - 30))) {
      switch1_light = true;
    }
  }

  // ******************** Evening Off ********************
  if ((Time.hour() == todaySleepHour) && (Time.minute() == todaySleepMinute)) {
      switch1_light = false;
  }
}


void checkDimmer() {
  if (start_dimmer) {
    // Stop after 30 minutes
    if (timeElapsed == (MINUTE * 30)) {
      dim = DIM_MIN;
      timeElapsed = 0;
      start_dimmer = false;
    }
    else {
      // dim one unit per minute
      dim = DIM_MAX - (timeElapsed/MINUTE);
    }
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  checkLight();
  checkDimmer();

  triggerSwitch1();
  triggerSwitch2();

  Blynk.run();
  timer.run();
}
