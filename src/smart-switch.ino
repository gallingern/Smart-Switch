#include <blynk.h>
#include "Calendar.h"
#include "Switch.h"
#include "ZeroCrossDimmer.h"

char auth[] = "4025c2c641f74330a7475890f4f42674";
int temp_f = 100;
int sunset_hour = 17; // 5pm
int sunset_minute = 0;
// Time in minutes past midnight
const int hour_to_minute = 60;
int wake_time = 6 * hour_to_minute + 30; // 6:30am
int sleep_time = 22 * hour_to_minute; // 10pm
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
  wake_time = t.getStartHour() * hour_to_minute + t.getStartMinute();
  sleep_time = t.getStopHour() * hour_to_minute + t.getStopMinute();
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
  int startAt = wake_time * 60;
  //seconds from the start of a day. 0 - min, 86399 - max
  int stopAt = sleep_time * 60;
  Blynk.virtualWrite(V3, startAt, stopAt, tz);

  // dimmer
  Blynk.virtualWrite(V4, ZeroCrossDimmer_getDimPercentage());
}


void setup() {
  ZeroCrossDimmer_init(Switch_lightOff);
  Switch_init();

  Particle.subscribe("temp", myTempHandler, MY_DEVICES);
  Particle.subscribe("hour", myHourHandler, MY_DEVICES);
  Particle.subscribe("minute", myMinuteHandler, MY_DEVICES);

  Blynk.begin(auth);

  // Setup a function to be called every second
  blynk_timer.setInterval(1000L, updateBlynk);
}


void checkLight() {
  int minutes_since_midnight = (Time.local() % 86400)/60;

  // ******************** Morning On ********************
  int today_wake_time = wake_time;
  // Weekend wake at 7:30am
  if (isWeekend()) {
    today_wake_time += (1 * hour_to_minute);
  }
  if ((minutes_since_midnight == today_wake_time) &&
      !ZeroCrossDimmer_isDimming()) {

    ZeroCrossDimmer_startDimOn();
    Switch_lightOn();

    // heat
    int threshold_f = 60; // degrees f
    if (temp_f < threshold_f) {
      Switch_heatOn();
    }
  }

  // ******************** Morning Off ********************
  int morning_off_time = 8 * hour_to_minute + 30; // 8:30am
  if (minutes_since_midnight == morning_off_time) {
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
  int today_sleep_time = sleep_time;
  // Weekend night sleep at 10:30pm
  if (isWeekendNight()) {
    today_sleep_time += (1 * hour_to_minute);
  }
  if (minutes_since_midnight == today_sleep_time) {
    ZeroCrossDimmer_startDimOff();
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  checkLight();

  Blynk.run();
  blynk_timer.run();
}
