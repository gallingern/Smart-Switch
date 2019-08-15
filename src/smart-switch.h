#ifndef SMART_SWITCH_H
#define SMART_SWITCH_H

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


// Warning, contains a hack
bool isDaylightSavingsTime() {
  int day_of_month = Time.day();
  int month = Time.month();
  int day_of_week = Time.weekday() - 1; // make Sunday 0 .. Saturday 6

  // April to October is DST
  if ((month >= APRIL) && (month <= OCTOBER)) {
    return true;
  }

  // March after second Sunday is DST
  if (month == MARCH) {
    // voodoo magic
    if ((day_of_month - day_of_week) > 8) {
      return true;
    }
  }

  // November before first Sunday is DST
  if (month == NOVEMBER) {
    // voodoo magic
    if (!((day_of_month - day_of_week) > 1)) {
      return true;
    }
  }

  return false;
}


// True if Saturday or Sunday
bool isWeekend() {
  int day_of_week = Time.weekday();
  return ((day_of_week == SATURDAY) || (day_of_week == SUNDAY));
}
// True if Friday or Saturday
bool isWeekendNight() {
  int day_of_week = Time.weekday();
  return ((day_of_week == FRIDAY) || (day_of_week == SATURDAY));
}


#endif /* SMART_SWITCH_H */
