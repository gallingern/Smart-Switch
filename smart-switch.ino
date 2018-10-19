#include <blynk.h>

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
const int PST_OFFSET  = -8;
const int PDT_OFFSET  = -7;

char auth[] = "";
int temp_f = 100;
int sunsetHour = 17;
int sunsetMinute = 0;
int wakeHour = 8;
int wakeMinute = 0;
bool switch1_light = false;
bool switch2_heat = false;
int nextPublish = millis();
int publishInterval = 10000; // 10 seconds


void myTempHandler(const char *event, const char *data) {
    temp_f = atoi(data);
}
void myHourHandler(const char *event, const char *data) {
    sunsetHour = atoi(data);
}
void myMinuteHandler(const char *event, const char *data) {
    sunsetMinute = atoi(data);
}


// TODO: Update blynk button state
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
}


void setup() {
    Serial.begin(9600);
    pinMode(SWITCH1_PIN, OUTPUT);
    pinMode(SWITCH2_PIN, OUTPUT);

    Particle.variable("temp_f", &temp_f, INT);
    Particle.variable("set_hour", &sunsetHour, INT);
    Particle.variable("set_minute", &sunsetMinute, INT);
    Particle.variable("wake_hour", &wakeHour, INT);
    Particle.variable("wake_minute", &wakeMinute, INT);
    Particle.subscribe("temp", myTempHandler, MY_DEVICES);
    Particle.subscribe("hour", myHourHandler, MY_DEVICES);
    Particle.subscribe("minute", myMinuteHandler, MY_DEVICES);
    Blynk.begin(auth);
}


// Warning, contains a hack
bool isDST() {
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


void checkLight() {
    // Wake trigger
    int weekday = Time.weekday();
    // Weekend wake at 8am, else take blynk input
    if ((weekday == 7) || (weekday == 1)) {
      wakeHour = 8;
      wakeMinute = 0;
    }
    if ((Time.hour() == wakeHour) && (Time.minute() == wakeMinute)) {
        switch1_light = true;
    }

    // Morning off trigger, 8:30a
    int morningHour = 8;
    int morningMinute = 30;
    if ((Time.hour() == morningHour) && (Time.minute() == morningMinute)) {
        switch1_light = false;
    }

    // 30 min before sunset trigger
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

    // Sleep trigger
    int sleepHour = 22;
    int sleepMinute = 30;
    if ((Time.hour() == sleepHour) && (Time.minute() == sleepMinute)) {
        switch1_light = false;
    }
}


void checkHeat() {
    int threshold_f = 60;
    int weekday = Time.weekday(); // 0-6
    int hour = Time.hour(); // 0-23
    int minute = Time.minute(); // 0 - 59

    // weekend
    if ((weekday == 7) || (weekday == 1)) {
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
    Time.zone(isDST() ? PDT_OFFSET : PST_OFFSET);

    checkLight();
    checkHeat();

    triggerSwitch1();
    triggerSwitch2();

    if (millis() > nextPublish) {
        Blynk.run();
        Blynk.virtualWrite(V0, temp_f);
        Blynk.virtualWrite(V1, switch1_light);
        Blynk.virtualWrite(V2, switch2_heat);
        nextPublish = millis() + publishInterval;
    }
}
