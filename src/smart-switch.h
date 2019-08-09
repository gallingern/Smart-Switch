#ifndef SMART_SWITCH_H
#define SMART_SWITCH_H

const int SWITCH1_PIN = D0;
const int SWITCH2_PIN = D1;
const int PST_OFFSET  = -8;
const int PDT_OFFSET  = -7;

enum weekday_names {
  SUNDAY = 1,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY
};

enum month_names {
  JANUARY = 1,
  FEBRUARY,
  MARCH,
  APRIL,
  MAY,
  JUNE,
  JULY,
  AUGUST,
  SEPTEMBER,
  OCTOBER,
  NOVEMBER,
  DECEMBER
};

#endif /* SMART_SWITCH_H */
