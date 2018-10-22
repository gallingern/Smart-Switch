#include <blynk.h>

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
const int PST_OFFSET  = -8;
const int PDT_OFFSET  = -7;

char auth[] = "";
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
  Serial.begin(9600);
  pinMode(SWITCH1_PIN, OUTPUT);
  pinMode(SWITCH2_PIN, OUTPUT);

  Particle.variable("temp_f", &temp_f, INT);
  Particle.variable("sunset_hr", &sunsetHour, INT);
  Particle.variable("sunset_min", &sunsetMinute, INT);
  Particle.variable("wake_hour", &wakeHour, INT);
  Particle.variable("wake_minute", &wakeMinute, INT);
  Particle.variable("sleep_hour", &sleepHour, INT);
  Particle.variable("sleep_minute", &sleepMinute, INT);

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
    switch1_light = true;
  }

  // Morning off trigger, 8:30a
  int morningOffHour = 8;
  int morningOffMinute = 30;
  // ******************** Morning Off ********************
  if ((Time.hour() == morningOffHour) && (Time.minute() == morningOffMinute)) {
    switch1_light = false;
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


void checkHeat() {
  int threshold_f = 60;
  int hour = Time.hour(); // 0-23
  int minute = Time.minute(); // 0 - 59

  // weekend
  if (isWeekend()) {
    // turn on at 7:00 AM
    if ((hour == 7) && (minute == 30) && (temp_f < threshold_f)) {
      switch2_heat = true;
    }
    // turn off at 8:00 AM
    if ((hour == 8) && (minute == 0)) {
      switch2_heat = false;
    }
  }

  // weekday
  else {
    // turn on at 6:00 AM
    if ((hour == 6) && (minute == 0) && (temp_f < threshold_f)) {
      switch2_heat = true;
    }
    // turn off at 7:00 AM
    if ((hour == 6) && (minute == 30)) {
      switch2_heat = false;
    }
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  checkLight();
  checkHeat();

  triggerSwitch1();
  triggerSwitch2();

  Blynk.run();
  timer.run();
}
