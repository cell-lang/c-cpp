#include "lib.h"


static uint8 non_leap_year_days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static uint8 leap_year_days_per_month[]     = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int16 months_offsets[]               = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333};


static bool is_leap_year(int32 year) {
  return ((year % 4 == 0) & (year % 100 != 0)) | (year % 400 == 0);
}

void get_year_month_day(int32 epoc_days, int32 *year_, int32 *month_, int32 *day_) {
  if (epoc_days >= 0) {
    int32 year = 1970;
    for ( ; ; ) {
      bool leap_year = is_leap_year(year);
      int32 year_len = leap_year ? 366 : 365;
      if (epoc_days < year_len) {
        uint8 *days_per_month = leap_year ? leap_year_days_per_month : non_leap_year_days_per_month;
        for (int32 month_idx=0 ; month_idx < 12 ; month_idx++) {
          int32 month_len = month_idx == 1 & leap_year ? 29 : days_per_month[month_idx];
          if (epoc_days < month_len) {
            *year_ = year;
            *month_ = month_idx + 1;
            *day_ = epoc_days + 1;
            return;
          }
          epoc_days = epoc_days - month_len;
        }
        internal_fail();
      }
      year = year + 1;
      epoc_days = epoc_days - year_len;
    }
  }
  else {
    int32 year = 1969;
    for ( ; ; ) {
      bool leap_year = is_leap_year(year);
      int32 year_len = leap_year ? 366 : 365;
      epoc_days = epoc_days + year_len;
      if (epoc_days >= 0) {
        uint8 *days_per_month = leap_year ? leap_year_days_per_month : non_leap_year_days_per_month;
        for (int32 month_idx=0 ; month_idx < 12 ; month_idx++) {
          int32 month_len = month_idx == 1 & leap_year ? 29 : days_per_month[month_idx];
          if (epoc_days < month_len) {
            *year_ = year;
            *month_ = month_idx + 1;
            *day_ = epoc_days + 1;
          }
          epoc_days = epoc_days - month_len;
        }
        internal_fail();
      }
      year = year - 1;
    }
  }
}

bool is_valid_date(int32 year, int32 month, int32 day) {
  if (month <= 0 | month > 12 | day <= 0)
    return false;

  bool leap_year = is_leap_year(year);

  if (month == 2 & day == 29)
    return leap_year;

  uint8 *days_per_month = leap_year ? leap_year_days_per_month : non_leap_year_days_per_month;
  return day <= days_per_month[month - 1];
}

bool date_time_is_within_range(int32 days_since_epoc, int64 day_time_ns) {
  // Valid range is from 1677-09-21 00:12:43.145224192 to 2262-04-11 23:47:16.854775807 inclusive
  return (days_since_epoc >  -106752 & days_since_epoc < 106751) |
         (days_since_epoc == -106752 & day_time_ns >= 763145224192L) |
         (days_since_epoc ==  106751 & day_time_ns <= 85636854775807L);
}

int32 days_since_epoc(int32 year, int32 month, int32 day) {
  assert(is_valid_date(year, month, day));

  bool leap_year = is_leap_year(year);

  int32 days_since_year_start;
  if (month == 2 & day == 29) {
    days_since_year_start = 59;
  }
  else {
    days_since_year_start = months_offsets[month - 1] + day;
    if (month > 2 & leap_year)
      days_since_year_start++;
  }

  int32 leap_years;
  if (year > 2000) {
    int32 delta_2001 = year - 2001;
    leap_years = 8 + delta_2001 / 4 - delta_2001 / 100 + delta_2001 / 400;
  }
  else {
    int32 delta_2000 = 2000 - year;
    leap_years = 7 - delta_2000 / 4 + delta_2000 / 100 - delta_2000 / 400;
  }

  int32 days_at_year_start = 365 * (year - 1970) + leap_years;
  return days_at_year_start + days_since_year_start;
}

int64 epoc_time_ns(int32 days_since_epoc, int64 day_time_ns) {
  return 86400000000000L * days_since_epoc + day_time_ns;
}
