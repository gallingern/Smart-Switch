#include <blynk.h>

#define SWITCH1_PIN     D0
#define SWITCH2_PIN     D1
#define PST_OFFSET      -8
#define PDT_OFFSET      -7

char auth[] = "<YOUR_AUTH_CODE_HERE>";
int temp_f = 100;
int sunsetHour = 17;
int sunsetMinute = 0;
bool switch1_light = false;
bool switch2_heat = false;
int nextPublish = millis();


void myTempHandler(const char *event, const char *data) {
    temp_f = atoi(data);
}
void myHourHandler(const char *event, const char *data) {
    sunsetHour = atoi(data);
}
void myMinuteHandler(const char *event, const char *data) {
    sunsetMinute = atoi(data);
}


// TO-DO: Update blynk button state
BLYNK_WRITE(V1) {
    if (param.asInt() == 0) { switch1_light = false; }
    else { switch1_light = true; }
}
BLYNK_WRITE(V2) {
    if (param.asInt() == 0) { switch2_heat = false; }
    else { switch2_heat = true; }
}


void setup() {
    Particle.syncTime();
    Time.zone(Time.isDST() ? PDT_OFFSET : PST_OFFSET);
    Serial.begin(9600);
    pinMode(SWITCH1_PIN, OUTPUT);
    pinMode(SWITCH2_PIN, OUTPUT);
    
    Particle.variable("temp_f", &temp_f, INT);
    Particle.variable("hour", &sunsetHour, INT);
    Particle.variable("minute", &sunsetMinute, INT);
    Particle.subscribe("temp", myTempHandler, MY_DEVICES);
    Particle.subscribe("hour", myHourHandler, MY_DEVICES);
    Particle.subscribe("minute", myMinuteHandler, MY_DEVICES);
    Blynk.begin(auth);
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
    // Weekend: 7am, Weekday: 6am
    int wakeHour = ((weekday == 7) || (weekday == 1)) ? 7 : 6;
    int wakeMinute = 0;
    if ((Time.hour() == wakeHour) && (Time.minute() == wakeMinute)) {
        switch1_light = true;
    }
    
    // Morning trigger
    int morningHour = 8;
    int morningMinute = 0;
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
    int sleepMinute = 0;
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
    if (millis() > nextPublish) {
        Blynk.run();
        Blynk.virtualWrite(V0, temp_f);
        Blynk.virtualWrite(V3, (int)switch1_light);
        Blynk.virtualWrite(V4, (int)switch2_heat);
        nextPublish = millis() + 10000;
    }
    
    checkLight();
    checkHeat();
    
    triggerSwitch1();
    triggerSwitch2();
}
