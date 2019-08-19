#include <blynk.h>
#include "Calendar.h"
#include "Switch.h"
#include "ZeroCrossDimmer.h"

char auth[] = "4025c2c641f74330a7475890f4f42674";
int temp_f = 100;
int threshold_f = 60; // degrees f
// Time in minutes past midnight
const int hour_to_minute = 60;
int wake_time = 6 * hour_to_minute + 30;   // 6:30am
int morning_off_time = 8 * hour_to_minute; // 8am
int sunset_time = (isDaylightSavingsTime() ? 19 : 17) * hour_to_minute; // 5-7pm
int sleep_time = 22 * hour_to_minute;      // 10pm
BlynkTimer blynk_timer;


// get info from other photon
void myTempHandler(const char *event, const char *data) {
  temp_f = atoi(data);
}
void mySunsetHandler(const char *event, const char *data) {
  sunset_time = atoi(data);
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
  ZeroCrossDimmer_init(Switch_lightOff, Switch_lightOn);
  Switch_init();

  Particle.subscribe("temp", myTempHandler, MY_DEVICES);
  Particle.subscribe("sunset", mySunsetHandler, MY_DEVICES);

  Blynk.begin(auth);

  // Setup a function to be called every second
  blynk_timer.setInterval(1000L, updateBlynk);
}


void switchStateMachine() {
  int minutes_since_midnight = (Time.local() % 86400)/60;

  // Don't interrupt dim on/off
  if (!ZeroCrossDimmer_isDimming()) {

    // ******************** Morning On ********************
    // Weekend morning wake an hour later
    if ((isWeekend() && (minutes_since_midnight == (wake_time + hour_to_minute))) ||
        (!isWeekend() && (minutes_since_midnight == wake_time))) {

      ZeroCrossDimmer_startDimOn();

      // heat
      if (temp_f < threshold_f) {
        Switch_heatOn();
      }
    }

    // ******************** Morning Off ********************
    if (minutes_since_midnight == morning_off_time) {

      ZeroCrossDimmer_startDimOff();

      Switch_heatOff();
    }

    // ******************** Evening On ********************
    // 30 min before sunset trigger
    if (minutes_since_midnight == (sunset_time - (hour_to_minute / 2))) {

      ZeroCrossDimmer_startDimOn();
    }

    // ******************** Evening Off ********************
    // Weekend night sleep an hour later
    if ((isWeekendNight() && (minutes_since_midnight == (sleep_time + hour_to_minute))) ||
       (!isWeekendNight() && (minutes_since_midnight == sleep_time))) {

      ZeroCrossDimmer_startDimOff();
    }
  }
}


void loop() {
  Time.zone(isDaylightSavingsTime() ? PDT_OFFSET : PST_OFFSET);

  switchStateMachine();

  Blynk.run();
  blynk_timer.run();
}
