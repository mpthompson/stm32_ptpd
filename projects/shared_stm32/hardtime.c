#include <math.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "hardtime.h"
#include "ntime.h"
#include "syslog.h"
#include "shell.h"

// Hardware time module.  Implements a 64-bit hardware timer independent
// of the disciplined system timer for near microsecond level timing of events.

// Hardware timer shell utility.
static bool hardtime_shell_hardtime(int argc, char **argv)
{
  int32_t seconds;
  int32_t nanoseconds;

  // Get the current time in seconds and nanoseconds.
  ntime_to_secs(ntime_get_nsecs(), &seconds, &nanoseconds);

  // Print the hardware timer value.
  shell_printf("%d.%06d\n", seconds, nanoseconds / 1000);

  return true;
}

// Initialize hardware timer resources.
void hardtime_init(void)
{
  // Set the shell command.
  shell_add_command("htime", hardtime_shell_hardtime);
}

// Get the current hardware timer timestamp.
int64_t hardtime_get(void)
{
  // Get the microsecond timer.
  return ntime_get_nsecs();
}

