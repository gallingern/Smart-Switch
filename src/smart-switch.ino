#include <blynk.h>
#include "Calendar.h"
#include "Switch.h"
#include "ZeroCrossDimmer.h"

char auth[] = "4025c2c641f74330a7475890f4f42674";
int temp_f = 100;
int sunset_hour = 17; // 5pm
int sunset_minute = 0;
int wake_hour = 6;
int wake_minute = 30;
int sleep_hour = 22; // 10pm
int sleep_minute = 0;
// replace time with std pair?
// std::pair<int,int> wake_time (6, 30);
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
  if (param.asInt() == 0) { Switch_lightOff(); }
  else { Switch_lightOn(); }
}
BLYNK_WRITE(V2) {
  if (param.asInt() == 0) { Switch_heatOff(); }
  else { Switch_heatOn(); }
}
BLYNK_WRITE(V3) {
  TimeInputParam t(param);
  wake_hour = t.getStartHour();
  wake_minute = t.getStartMinute();
  sleep_hour = t.getStopHour();
  sleep_minute = t.getStopMinute();
}
BLYNK_WRITE(V4) {
  ZeroCrossDimmer_setDimPercentage(param.asInt());
}

void updateBlynk() {
  // send temp to blynk app
  Blynk.virtualWrite(V0, temp_f);
  // update button states on blynk app
  Blynk.virtualWrite(V1, Switch_getLightState());
  Blynk.virtualWrite(V2, Switch_getHeatState());

  // Update time input
  // timezone
  char tz[] = "US/Pacific";
  //seconds from the start of a day. 0 - min, 86399 - max
  int startAt = ((wake_hour*60) + wake_minute) * 60;
  //seconds from the start of a day. 0 - min, 86399 - max
  int stopAt = ((sleep_hour*60) + sleep_minute) * 60;
  Blynk.virtualWrite(V3, startAt, stopAt, tz);

  // dimmer
  Blynk.virtualWrite(V4, ZeroCrossDimmer_getDimPercentage());
}


void setup() {
  ZeroCrossDimmer_init(Switch_lightOff);
  Switch_init();

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


void checkLight() {
  // ******************** Morning On ********************
  int today_wake_hour = wake_hour;
  int today_wake_minute = wake_minute;
  // Weekend wake at 7:30am
  if (isWeekend()) {
    today_wake_hour = 7;
    today_wake_minute = 30;
  }
  if ((Time.hour() == today_wake_hour) &&
      (Time.minute() == today_wake_minute) &&
      (!ZeroCrossDimmer_isDimming())) {

    ZeroCrossDimmer_startDimOn();
    Switch_lightOn();

    // heat
    int threshold_f = 60; // degrees f
    if (temp_f < threshold_f) {
      Switch_heatOn();
    }
  }

  // ******************** Morning Off ********************
  int morning_off_hour = 8;
  int morning_off_minute = 30;
  if ((Time.hour() == morning_off_hour) && (Time.minute() == morning_off_minute)) {
    Switch_lightOff();
    Switch_heatOff();
  }

  // ******************** Evening On ********************
  // 30 min before sunset trigger (also a hack)
  if (sunset_minute < 30) {
    if ((Time.hour() == (sunset_hour - 1)) && (Time.minute() == (sunset_minute + 30))) {
      Switch_lightOn();
    }
  }
  else {
    if ((Time.hour() == sunset_hour) && (Time.minute() == (sunset_minute - 30))) {
      Switch_lightOn();
    }
  }

  // ******************** Evening Off ********************
  int today_sleep_hour = sleep_hour;
  int today_sleep_minute = sleep_minute;
  // Weekend night sleep at 10:30pm
  if (isWeekendNight()) {
    today_sleep_hour = 22;
    today_sleep_minute = 30;
  }
  if ((Time.hour() == today_sleep_hour) && (Time.minute() == today_sleep_minute)) {
    ZeroCrossDimmer_startDimOff();
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  checkLight();

  Blynk.run();
  blynk_timer.run();
}
