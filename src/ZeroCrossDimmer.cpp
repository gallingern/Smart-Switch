#include "elapsedMillis.h"
#include "SparkIntervalTimer.h"
#include "ZeroCrossDimmer.h"

const int triac_period = 65;           // micro seconds
const int dimmer_period = 2000;        // half milliseconds (equals 1 second)
const int AC_PIN = D3;                 // Output to Opto Triac
const int PIN_ZERO_CROSS = D2;         // Interrrupt pin
// Min/Max depends on your circuit
// This is for my bedside LED lights
const int DIM_MIN = 0;                 // DIM_MIN == light on
const int DIM_THRESHOLD = 100;         // on to off
const int DIM_MAX = 128;               // DIM_MAX == lights off
const int MINUTE = 60000;              // minute in milliseconds
volatile bool zero_cross = false;      // Boolean to store a "switch" to tell us if we have crossed zero
volatile bool dim_on = false;          // trigger to start 30 minute dim on
volatile bool dim_off = false;         // trigger to start 30 minute dim off
volatile int counter = 0;              // Variable to use as a counter volatile as it is in an interrupt
volatile int dim_percentage = DIM_MIN; // Dimming level (0-128)  0 = on, 128 = 0ff

callback_function switchLightOff;
callback_function switchLightOn;

IntervalTimer timer_triac;
IntervalTimer timer_dimmer;
elapsedMillis time_elapsed;


void ZeroCrossDimmer_init(callback_function _switchLightOff,
                          callback_function _switchLightOn) {
  pinMode(PIN_ZERO_CROSS, INPUT);
  pinMode(AC_PIN, OUTPUT);
  timer_triac.begin(ZeroCrossDimmer_enableTRIAC, triac_period, uSec, TIMER5);
  timer_dimmer.begin(ZeroCrossDimmer_dim, dimmer_period, hmSec);
  attachInterrupt(PIN_ZERO_CROSS, ZeroCrossDimmer_detectCross, RISING);
  switchLightOff = _switchLightOff;
  switchLightOn = _switchLightOn;
}


// Check for AC zero cross
void ZeroCrossDimmer_detectCross() {
  zero_cross = true;         // set the boolean to true to tell our dimming function that a zero cross has occured
  counter = 0;
  digitalWrite(AC_PIN, LOW); // turn off TRIAC (and AC)
}


// Turn on the TRIAC at the appropriate time
void ZeroCrossDimmer_enableTRIAC() {
  if (zero_cross == true) {
    if (counter >= dim_percentage) {
      digitalWrite(AC_PIN, HIGH); // turn on light
      counter = 0;                // reset time step counter
      zero_cross = false;         //reset zero cross detection
    }
    else {
      counter++; // increment time step counter
    }
  }
}


void ZeroCrossDimmer_startDimOn() {
  dim_percentage = DIM_THRESHOLD;
  time_elapsed = 0;
  dim_on = true;
  switchLightOn();
}


void ZeroCrossDimmer_startDimOff() {
  dim_percentage = DIM_MIN;
  time_elapsed = 0;
  dim_off = true;
}


bool ZeroCrossDimmer_isDimming() {
  return (dim_on || dim_off);
}
int ZeroCrossDimmer_getDimPercentage() {
  return dim_percentage;
}
void ZeroCrossDimmer_setDimPercentage(int percentage) {
  dim_percentage = percentage;
}


void ZeroCrossDimmer_dim() {
  if (dim_on) {
    // Stop after 30 minutes
    if (time_elapsed == (MINUTE * 30)) {
      dim_percentage = DIM_MIN;
      time_elapsed = 0;
      dim_on = false;
    }
    else {
      // dim one unit per minute
      dim_percentage = DIM_THRESHOLD - (time_elapsed/MINUTE);
    }
  }

  if (dim_off) {
    // Stop after 30 minutes
    if (time_elapsed == (MINUTE * 30)) {
      dim_percentage = DIM_MAX;
      time_elapsed = 0;
      dim_off = false;
      switchLightOff();
    }
    else {
      // dim one unit per minute
      dim_percentage = DIM_MIN + (time_elapsed/MINUTE);
    }
  }
}
