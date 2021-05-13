#include <time.h>
#include "cmsis_os2.h"
#include "systime.h"
#include "shell.h"
#include "outputf.h"
#include "ethptp.h"

#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
#define _localtime_r localtime_r
#endif

// Indicates when system time becomes valid.
static bool systime_valid = false;

// Name of days.
static char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Name of months.
static char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// Return the time/date relative to the high precision Ethernet timer.
static bool systime_shell_date(int argc, char **argv)
{
  char buffer[32];

  // Get the date from system time.
  systime_str(buffer, sizeof(buffer));

  // Print the string.
  shell_printf("%s\n", buffer);

  return true;
}

// Intialize the system time.  This must be called after
// Ethernet hardware is initialized.
void systime_init(void)
{
  ptptime_t ptptime;

  // Initialize Ethernet clock to Wednesday, January 1, 2020 12:00:00 AM (GMT).
  // See: https://www.epochconverter.com/
  ptptime.tv_sec = 1577836800;
  ptptime.tv_nsec = 0;
  ethptp_set_time(&ptptime);

  // Add the shell command to print the date.
  shell_add_command("date", systime_shell_date);

  // Consider system time valid.
  // XXX This actually should only be set once
  // XXX PTPD has established the system time.
  systime_valid = true;
}

// Returns true of system time is considered valid.
bool systime_is_valid(void)
{
  return systime_valid;
}

// Get the system time as nanoseconds since 1970.  We must be able to call this
// function from interrupt service routines for certain functionality.
int64_t systime_get(void)
{
  int64_t systime;
  ptptime_t ptptime;

  // Get the Ethernet time values.
  ethptp_get_time(&ptptime);

  // Get seconds since 1970 (Unix epoch).
  systime = (uint64_t) ptptime.tv_sec;

#ifdef USE_SYSTIME_GPS_EPOCH
  // Adjust to time since 1980 (GPS epoch). There was an offset of 315964800 seconds
  // between Unix and GPS time when GPS time began, that offset changes each time
  // there is a leap second. We currently don't factor in leap seconds.
  // See: https://www.andrews.edu/~tzs/timeconv/timealgorithm.html
  systime -= 315964800;
#endif

  // Add in the nanosecond value.
  systime = (systime * 1000000000) + ptptime.tv_nsec;

  // Return the system time.
  return systime;
}

// Set the system time as nanoseconds since 1970.
void systime_set(int64_t systime)
{
  ptptime_t ptptime;

  // Get the nanoseconds portion of the time.
  ptptime.tv_nsec = systime % 1000000000;

  // Adjust to the seconds.
  systime = systime / 1000000000;

#ifdef USE_SYSTIME_GPS_EPOCH
  // Adjust to time since 1970 (Unix epoch). There was an offset of 315964800 seconds
  // between Unix and GPS time when GPS time began, that offset changes each time
  // there is a leap second. We currently don't factor in leap seconds.
  // See: https://www.andrews.edu/~tzs/timeconv/timealgorithm.html
  systime += 315964800;
#endif

  // Get the seconds portion of the time.
  ptptime.tv_sec = (int32_t) systime;

  // Set the system time.
  ethptp_set_time(&ptptime);
}

// Adjust the system clock rate by the indicated value in parts-per-billion.
void systime_adjust(int32_t adjustment)
{
  // Pass the adjustment to ethernet clock.
  ethptp_adj_freq(adjustment);
}

// Create a formatted system time similar to strftime().
size_t systime_str(char *buffer, size_t buflen)
{
  time_t seconds1970;
  struct tm now;
  ptptime_t ptptime;

  // Get the ethernet time values.
  ethptp_get_time(&ptptime);

  // Get the seconds since 1970 (Unix epoch).
  seconds1970 = (time_t) ptptime.tv_sec;

  // Break the seconds to a time structure.
  _localtime_r(&seconds1970, &now);

  // Format into a string.  We don't use strftime() to reduce stack usage.
  // return strftime(buffer, buflen, "%a %b %d %H:%M:%S UTC %Y\n", &now);
  return snoutputf(buffer, buflen, "%s %s %02d %02d:%02d:%02d UTC %4d",
                   days[now.tm_wday], months[now.tm_mon], now.tm_mday,
                   now.tm_hour, now.tm_min, now.tm_sec, 1900 + now.tm_year);
}

// Create a formatted log time similar to strftime().
size_t systime_log(char *buffer, size_t buflen)
{
  time_t seconds1970;
  struct tm now;
  ptptime_t ptptime;

  // Get the ethernet time values.
  ethptp_get_time(&ptptime);

  // Get the seconds since 1970 (Unix epoch).
  seconds1970 = (time_t) ptptime.tv_sec;

  // Break the seconds to a time structure.
  _localtime_r(&seconds1970, &now);

  // Format into a string.  We don't use strftime() to reduce stack usage.
  // return strftime(buffer, buflen, "%b %e %H:%M:%S firmware: ", &now);
  return snoutputf(buffer, buflen, "%s %2d %02d:%02d:%02d firmware: ",
                   months[now.tm_mon], now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
}

