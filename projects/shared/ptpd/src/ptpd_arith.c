#include "ptpd.h"

#if LWIP_PTPD

// Normalize the time making sure there are never more
// than a billion nanoseconds represented.
static void ptpd_normalize_time(TimeInternal *r)
{
  r->seconds += r->nanoseconds / 1000000000;
  r->nanoseconds -= r->nanoseconds / 1000000000 * 1000000000;

  if ((r->seconds > 0) && (r->nanoseconds < 0))
  {
    r->seconds -= 1;
    r->nanoseconds += 1000000000;
  }
  else if ((r->seconds < 0) && (r->nanoseconds > 0))
  {
    r->seconds += 1;
    r->nanoseconds -= 1000000000;
  }
}

// Convert scaled nanoseconds into TimeInternal structure.
void ptpd_scaled_nanoseconds_to_internal_time(TimeInternal *internal, const int64_t *scaled_nonoseconds)
{
  int sign;
  int64_t nanoseconds = *scaled_nonoseconds;

  // Determine sign of result big integer number.
  if (nanoseconds < 0)
  {
    nanoseconds = -nanoseconds;
    sign = -1;
  }
  else
  {
    sign = 1;
  }

  // Fractional nanoseconds are excluded (see 5.3.2).
  nanoseconds >>= 16;
  internal->seconds = sign * (nanoseconds / 1000000000);
  internal->nanoseconds = sign * (nanoseconds % 1000000000);
}

// Convert TimeInternal into Timestamp structure (defined by the spec).
void ptpd_from_internal_time(const TimeInternal *internal, Timestamp *external)
{
  // ptpd_from_internal_time() is only used to convert time given by the system
  // to a timestamp. As a consequence, no negative value can normally be found
  // in (internal). Note that offsets are also represented with TimeInternal
  // structure, and can be negative, but offset are never convert into Timestamp
  // so there is no problem here.
  if ((internal->seconds & ~INT_MAX) || (internal->nanoseconds & ~INT_MAX))
  {
    DBG("PTPD: Negative value cannot be converted into timestamp \n");
    return;
  }
  else
  {
    external->secondsField.lsb = internal->seconds;
    external->nanosecondsField = internal->nanoseconds;
    external->secondsField.msb = 0;
  }
}

// Convert Timestamp to TimeInternal structure (defined by the spec).
void ptpd_to_internal_time(TimeInternal *internal, const Timestamp *external)
{
  // NOTE: Program will not run after 2038...
  if (external->secondsField.lsb < INT_MAX)
  {
    internal->seconds = external->secondsField.lsb;
    internal->nanoseconds = external->nanosecondsField;
  }
  else
  {
    DBG("PTPD: Clock servo cannot be executed: seconds field is higher than signed integer (32bits)\n");
    return;
  }
}

// Add two TimeInternal structure and normalize.
void ptpd_add_time(TimeInternal *r, const TimeInternal *x, const TimeInternal *y)
{
  r->seconds = x->seconds + y->seconds;
  r->nanoseconds = x->nanoseconds + y->nanoseconds;

  ptpd_normalize_time(r);
}

// Substract two TimeInternal structure and normalize.
void ptpd_sub_time(TimeInternal *r, const TimeInternal *x, const TimeInternal *y)
{
  r->seconds = x->seconds - y->seconds;
  r->nanoseconds = x->nanoseconds - y->nanoseconds;

  ptpd_normalize_time(r);
}

// Divide the TimeInternal by 2 and normalize.
void ptpd_div2_time(TimeInternal *r)
{
  r->nanoseconds += r->seconds % 2 * 1000000000;
  r->seconds /= 2;
  r->nanoseconds /= 2;

  ptpd_normalize_time(r);
}

// Returns the floor form of binary logarithm for a 32 bit integer.
// -1 is returned if 'n' is 0.
int32_t ptpd_floor_log2(uint32_t n)
{
  int pos = 0;

  // Sanity check.
  if (n == 0) return -1;

  if (n >= 1 << 16) { n >>= 16; pos += 16; }
  if (n >= 1 <<  8) { n >>=  8; pos +=  8; }
  if (n >= 1 <<  4) { n >>=  4; pos +=  4; }
  if (n >= 1 <<  2) { n >>=  2; pos +=  2; }
  if (n >= 1 <<  1) {           pos +=  1; }

  return pos;
}

#endif // LWIP_PTPD
