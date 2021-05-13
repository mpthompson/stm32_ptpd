#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "console.h"
#include "outputf.h"

// Definition to outputf the float number.
#ifndef __OUTPUTF_FLOAT_ENABLE
#define __OUTPUTF_FLOAT_ENABLE 1
#endif

// Definition to inputf the float number.
#ifndef __INPUTF_FLOAT_ENABLE
#define __INPUTF_FLOAT_ENABLE 1
#endif

// Definition to support advanced format specifier for outputf.
#ifndef __OUTPUTF_ADVANCED_ENABLE
#define __OUTPUTF_ADVANCED_ENABLE 1
#endif

// Definition to support advanced format specifier for inputf.
#ifndef __INPUTF_ADVANCED_ENABLE
#define __INPUTF_ADVANCED_ENABLE 1
#endif

// The overflow value.
#ifndef HUGE_VAL
#define HUGE_VAL (99.e99)
#endif

#if __INPUTF_FLOAT_ENABLE
static double fnum = 0.0;
#endif

#if __OUTPUTF_ADVANCED_ENABLE
// Specification modifier flags for outputf.
enum
{
  OUTPUTF_Flag_Minus = 0x01U,              // Minus FLag.
  OUTPUTF_Flag_Plus = 0x02U,               // Plus Flag.
  OUTPUTF_Flag_Space = 0x04U,              // Space Flag.
  OUTPUTF_Flag_Zero = 0x08U,               // Zero Flag.
  OUTPUTF_Flag_Pound = 0x10U,              // Pound Flag.
  OUTPUTF_Flag_LengthChar = 0x20U,         // Length: Char Flag.
  OUTPUTF_Flag_LengthShortInt = 0x40U,     // Length: Short Int Flag.
  OUTPUTF_Flag_LengthLongInt = 0x80U,      // Length: Long Int Flag.
  OUTPUTF_Flag_LengthLongLongInt = 0x100U, // Length: Long Long Int Flag.
};
#endif

// Specification modifier flags for inputf.
enum
{
  INPUTF_Flag_Suppress = 0x2U,             // Suppress Flag.
  INPUTF_Flag_DestMask = 0x7cU,            // Destination Mask.
  INPUTF_Flag_DestChar = 0x4U,             // Destination Char Flag.
  INPUTF_Flag_DestString = 0x8U,           // Destination String FLag.
  INPUTF_Flag_DestSet = 0x10U,             // Destination Set Flag.
  INPUTF_Flag_DestInt = 0x20U,             // Destination Int Flag.
  INPUTF_Flag_DestFloat = 0x30U,           // Destination Float Flag.
  INPUTF_Flag_LengthMask = 0x1f00U,        // Length Mask Flag.
#if __INPUTF_ADVANCED_ENABLE
  INPUTF_Flag_LengthChar = 0x100U,         // Length Char Flag.
  INPUTF_Flag_LengthShortInt = 0x200U,     // Length ShortInt Flag.
  INPUTF_Flag_LengthLongInt = 0x400U,      // Length LongInt Flag.
  INPUTF_Flag_LengthLongLongInt = 0x800U,  // Length LongLongInt Flag.
#endif
#if __OUTPUTF_FLOAT_ENABLE
  INPUTF_Flag_LengthLongLongDouble = 0x1000U, // Length LongLongDuoble Flag.
#endif
  INPUTF_Flag_TypeSinged = 0x2000U,        // TypeSinged Flag.
};

// Define the structure that tracks filling a string into a string buffer.
typedef struct output_ch_s
{
  uint16_t index;
  uint16_t buflen;
  char *buffer;
  union
  {
    putc_func_t no_arg;
    putc_func_with_arg_t with_arg;
  } putc_func;
  void *putc_func_arg;
} output_ch_t;

// Initialize the structure that safely tracks filling a string buffer.
static void output_init(output_ch_t *output, char *buffer, uint16_t buflen, putc_func_t func)
{
  // Initialize the output structure.
  output->index = 0;
  output->buflen = buflen;
  output->buffer = buffer;
  output->putc_func.no_arg = func;
  output->putc_func_arg = NULL;
}

// Initialize the structure that safely tracks filling a string buffer.
static void output_init_with_arg(output_ch_t *output, char *buffer, uint16_t buflen, putc_func_with_arg_t func, void *func_arg)
{
  // Initialize the output structure.
  output->index = 0;
  output->buflen = buflen;
  output->buffer = buffer;
  output->putc_func.with_arg = func;
  output->putc_func_arg = func_arg;
}

// Insert a single character.
static void output_ch(output_ch_t *output, char ch)
{
  // Do we have a buffer?
  if (output->buffer != NULL)
  {
    // Prevent buffer overflow.
    if (output->index < (output->buflen - 1))
    {
      output->buffer[output->index++] = ch;
    }
  }
  else if (output->putc_func.with_arg != NULL)
  {
    // Call the output function. Notice we call the same function
    // with the argument, even if the callee doesn't use the argument.
    // We can do this because we are relying on the standard C function
    // call mechanism by which the caller cleans up the call stack.

    // Output a carriage return character with a new line.
    if (ch == '\n') { output->putc_func.with_arg('\r', output->putc_func_arg); output->index++; }

    // Output the character.
    output->putc_func.with_arg(ch, output->putc_func_arg);

    // Count characters output.
    output->index++;
  }
  else
  {
    // Output a carriage return character with a new line.
    if (ch == '\n') { console_putc('\r'); output->index++; }

    // Output the character out the console port.
    console_putc(ch);

    // Count characters output.
    output->index++;
  }
}

// Wrap up inserting a string into the string buffer.
static void output_end(output_ch_t *output)
{
  // Do we have a buffer?
  if (output->buffer != NULL)
  {
    // Null terminate.
    output->buffer[output->index] = 0;
  }
}

// Scanline function which ignores white spaces.
static uint32_t ignore_white_space(const char **s)
{
    uint8_t count = 0;
    uint8_t c;

    c = **s;
    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\v') || (c == '\f'))
    {
        count++;
        (*s)++;
        c = **s;
    }
    return count;
}

// This function puts padding character.
static void output_padding(output_ch_t *output, char c, int32_t curlen, int32_t width, int32_t *count)
{
  int32_t i;

  for (i = curlen; i < width; i++)
  {
    output_ch(output, c);
    (*count)++;
  }
}

// Converts a radix number to a string and return its length.
static int32_t radix_num_to_string(char *numstr, void *nump, int32_t neg, int32_t radix, bool use_caps)
{
#if __OUTPUTF_ADVANCED_ENABLE
  int64_t a;
  int64_t b;
  int64_t c;
  uint64_t ua;
  uint64_t ub;
  uint64_t uc;
#else
  int32_t a;
  int32_t b;
  int32_t c;
  uint32_t ua;
  uint32_t ub;
  uint32_t uc;
#endif

  int32_t nlen;
  char *nstrp;

  nlen = 0;
  nstrp = numstr;
  *nstrp++ = '\0';

  if (neg)
  {
#if __OUTPUTF_ADVANCED_ENABLE
    a = *(int64_t *) nump;
#else
    a = *(int32_t *) nump;
#endif
    if (a == 0)
    {
      *nstrp = '0';
      ++nlen;
      return nlen;
    }
    while (a != 0)
    {
#if __OUTPUTF_ADVANCED_ENABLE
      b = (int64_t) a / (int64_t) radix;
      c = (int64_t) a - ((int64_t) b * (int64_t) radix);
      if (c < 0)
      {
        uc = (uint64_t) c;
        c = (int64_t) (~uc) + 1 + '0';
      }
#else
      b = a / radix;
      c = a - (b * radix);
      if (c < 0)
      {
        uc = (uint32_t) c;
        c = (uint32_t) (~uc) + 1 + '0';
      }
#endif
      else
      {
        c = c + '0';
      }
      a = b;
      *nstrp++ = (char) c;
      ++nlen;
    }
  }
  else
  {
#if __OUTPUTF_ADVANCED_ENABLE
    ua = *(uint64_t *) nump;
#else
    ua = *(uint32_t *) nump;
#endif
    if (ua == 0)
    {
      *nstrp = '0';
      ++nlen;
      return nlen;
    }
    while (ua != 0)
    {
#if __OUTPUTF_ADVANCED_ENABLE
      ub = (uint64_t) ua / (uint64_t) radix;
      uc = (uint64_t) ua - ((uint64_t) ub * (uint64_t) radix);
#else
      ub = ua / (uint32_t) radix;
      uc = ua - (ub * (uint32_t)radix);
#endif

      if (uc < 10)
      {
        uc = uc + '0';
      }
      else
      {
        uc = uc - 10 + (use_caps ? 'A' : 'a');
      }
      ua = ub;
      *nstrp++ = (char)uc;
      ++nlen;
    }
  }
  return nlen;
}

#if __OUTPUTF_FLOAT_ENABLE
// Converts a floating radix number to a string and return its length.
static int32_t radix_float_to_string(char *numstr, void *nump, int32_t radix, uint32_t precision_width)
{
  int32_t a;
  int32_t b;
  int32_t c;
  int32_t i;
  int32_t nlen;
  uint32_t uc;
  char *nstrp;
  double fa;
  double dc;
  double fb;
  double r;
  double fractpart;
  double intpart;

  nlen = 0;
  nstrp = numstr;
  *nstrp++ = '\0';
  r = *((double *) nump);
#if 0
  if (!r)
  {
    *nstrp = '0';
    ++nlen;
    return nlen;
  }
#endif
  fractpart = modf((double) r, (double *) &intpart);

  // Process fractional part.
  for (i = 0; i < precision_width; i++)
  {
    fractpart *= radix;
  }
  if (r >= 0)
  {
    fa = fractpart + (double) 0.5;
    if (fa >= pow(10, precision_width))
    {
      intpart++;
    }
  }
  else
  {
    fa = fractpart - (double) 0.5;
    if (fa <= -pow(10, precision_width))
    {
      intpart--;
    }
  }
  if (precision_width > 0)
  {
    for (i = 0; i < precision_width; i++)
    {
      fb = fa / (int32_t) radix;
      dc = (fa - (int64_t) fb * (int32_t) radix);
      c = (int32_t) dc;
      if (c < 0)
      {
        uc = (uint32_t) c;
        c = (int32_t) (~uc) + 1 + '0';
      }
      else
      {
        c = c + '0';
      }
      fa = fb;
      *nstrp++ = (char) c;
      ++nlen;
    }
    *nstrp++ = (char) '.';
    ++nlen;
  }
  a = (int32_t)intpart;
  if (a == 0)
  {
    *nstrp++ = '0';
    ++nlen;
  }
  else
  {
    while (a != 0)
    {
      b = (int32_t) a / (int32_t) radix;
      c = (int32_t) a - ((int32_t) b * (int32_t) radix);
      if (c < 0)
      {
        uc = (uint32_t) c;
        c = (int32_t) (~uc) + 1 + '0';
      }
      else
      {
        c = c + '0';
      }
      a = b;
      *nstrp++ = (char) c;
      ++nlen;
    }
  }
  return nlen;
}
#endif

// Format the characters into the string buffer.  We do our own
// minimal implementation of "printf" style formatting to reduce
// stack usage.
static int format(output_ch_t *output, const char *fmt, va_list ap)
{
  char *p;
  char vstr[33];
  char *vstrp = NULL;
  int32_t c;
  int32_t done;
  int32_t vlen = 0;
  int32_t count = 0;

  char *sval;
  int32_t cval;
  bool use_caps;
  uint8_t radix = 0;
  uint32_t field_width;
  uint32_t precision_width;

#if __OUTPUTF_ADVANCED_ENABLE
  uint32_t flags_used;
  int32_t schar, dschar;
  int64_t ival;
  uint64_t uval = 0;
  bool valid_precision_width;
#else
  int32_t ival;
  uint32_t uval = 0;
#endif

#if __OUTPUTF_FLOAT_ENABLE
  double fval;
#endif

  // Start parsing apart the format string and display appropriate formats and data.
  for (p = (char *) fmt; (c = *p) != 0; p++)
  {
    // All formats begin with a '%' marker.  Special chars like
    // '\n' or '\t' are normally converted to the appropriate
    // character by the __compiler__.  Thus, no need for this
    // routine to account for the '\' character.
    if (c != '%')
    {
      output_ch(output, c);
      count++;
      // By using 'continue', the next iteration of the loop
      // is used, skipping the code that follows.
      continue;
    }

    use_caps = true;

#if __OUTPUTF_ADVANCED_ENABLE
    // First check for specification modifier flags.
    flags_used = 0;
    done = false;
    while (!done)
    {
      switch (*++p)
      {
        case '-':
          flags_used |= OUTPUTF_Flag_Minus;
          break;
        case '+':
          flags_used |= OUTPUTF_Flag_Plus;
          break;
        case ' ':
          flags_used |= OUTPUTF_Flag_Space;
          break;
        case '0':
          flags_used |= OUTPUTF_Flag_Zero;
          break;
        case '#':
          flags_used |= OUTPUTF_Flag_Pound;
          break;
        default:
          // We've gone one char too far.
          --p;
          done = true;
          break;
      }
    }
#endif

    // Next check for minimum field width.
    field_width = 0;
    done = false;
    while (!done)
    {
      c = *++p;
      if ((c >= '0') && (c <= '9'))
      {
        field_width = (field_width * 10) + (c - '0');
      }
#if __OUTPUTF_ADVANCED_ENABLE
      else if (c == '*')
      {
        field_width = (uint32_t)va_arg(ap, uint32_t);
      }
#endif
      else
      {
        // We've gone one char too far.
        --p;
        done = true;
      }
    }

    // Next check for the width and precision field separator.
    precision_width = 6;
#if __OUTPUTF_ADVANCED_ENABLE
    valid_precision_width = false;
#endif
    if (*++p == '.')
    {
      // Must get precision field width, if present.
      precision_width = 0;
      done = false;
      while (!done)
      {
        c = *++p;
        if ((c >= '0') && (c <= '9'))
        {
          precision_width = (precision_width * 10) + (c - '0');
#if __OUTPUTF_ADVANCED_ENABLE
          valid_precision_width = true;
#endif
        }
#if __OUTPUTF_ADVANCED_ENABLE
        else if (c == '*')
        {
          precision_width = (uint32_t)va_arg(ap, uint32_t);
          valid_precision_width = true;
        }
#endif
        else
        {
          // We've gone one char too far.
          --p;
          done = true;
        }
      }
    }
    else
    {
      // We've gone one char too far.
      --p;
    }
#if __OUTPUTF_ADVANCED_ENABLE
    // Check for the length modifier.
    switch (*++p)
    {
      case 'h':
        if (*++p != 'h')
        {
          flags_used |= OUTPUTF_Flag_LengthShortInt;
          --p;
        }
        else
        {
          flags_used |= OUTPUTF_Flag_LengthChar;
        }
        break;
      case 'l':
        if (*++p != 'l')
        {
          flags_used |= OUTPUTF_Flag_LengthLongInt;
          --p;
        }
        else
        {
          flags_used |= OUTPUTF_Flag_LengthLongLongInt;
        }
        break;
      default:
        // We've gone one char too far.
        --p;
        break;
    }
#endif
    // Now we're ready to examine the format.
    c = *++p;
    {
      if ((c == 'd') || (c == 'i') || (c == 'f') || (c == 'F') || (c == 'x') || (c == 'X') || (c == 'o') ||
          (c == 'b') || (c == 'p') || (c == 'u'))
      {
        if ((c == 'd') || (c == 'i'))
        {
#if __OUTPUTF_ADVANCED_ENABLE
          if (flags_used & OUTPUTF_Flag_LengthLongLongInt)
          {
            ival = (int64_t)va_arg(ap, int64_t);
          }
          else
#endif
          {
            ival = (int32_t)va_arg(ap, int32_t);
          }
          vlen = radix_num_to_string(vstr, &ival, true, 10, use_caps);
          vstrp = &vstr[vlen];
#if __OUTPUTF_ADVANCED_ENABLE
          if (ival < 0)
          {
            schar = '-';
            ++vlen;
          }
          else
          {
            if (flags_used & OUTPUTF_Flag_Plus)
            {
              schar = '+';
              ++vlen;
            }
            else
            {
              if (flags_used & OUTPUTF_Flag_Space)
              {
                schar = ' ';
                ++vlen;
              }
              else
              {
                schar = 0;
              }
            }
          }
          dschar = false;
          // Do the ZERO pad.
          if (flags_used & OUTPUTF_Flag_Zero)
          {
            if (schar)
            {
              output_ch(output, schar);
              count++;
            }
            dschar = true;

            output_padding(output, '0', vlen, field_width, &count);
            vlen = field_width;
          }
          else
          {
            if (!(flags_used & OUTPUTF_Flag_Minus))
            {
              output_padding(output, ' ', vlen, field_width, &count);
              if (schar)
              {
                output_ch(output, schar);
                count++;
              }
              dschar = true;
            }
          }
          // The string was built in reverse order, now display in correct order.
          if ((!dschar) && schar)
          {
            output_ch(output, schar);
            count++;
          }
#endif
        }

#if __OUTPUTF_FLOAT_ENABLE
        if ((c == 'f') || (c == 'F'))
        {
          fval = (double) va_arg(ap, double);
          vlen = radix_float_to_string(vstr, &fval, 10, precision_width);
          vstrp = &vstr[vlen];

#if __OUTPUTF_ADVANCED_ENABLE
          if (fval < 0)
          {
            schar = '-';
            ++vlen;
          }
          else
          {
            if (flags_used & OUTPUTF_Flag_Plus)
            {
              schar = '+';
              ++vlen;
            }
            else
            {
              if (flags_used & OUTPUTF_Flag_Space)
              {
                schar = ' ';
                ++vlen;
              }
              else
              {
                schar = 0;
              }
            }
          }
          dschar = false;
          if (flags_used & OUTPUTF_Flag_Zero)
          {
            if (schar)
            {
              output_ch(output, schar);
              count++;
            }
            dschar = true;
            output_padding(output, '0', vlen, field_width, &count);
            vlen = field_width;
          }
          else
          {
            if (!(flags_used & OUTPUTF_Flag_Minus))
            {
              output_padding(output, ' ', vlen, field_width, &count);
              if (schar)
              {
                output_ch(output, schar);
                count++;
              }
              dschar = true;
            }
          }
          if ((!dschar) && schar)
          {
            output_ch(output, schar);
            count++;
          }
#endif
        }
#endif
        if ((c == 'X') || (c == 'x'))
        {
          if (c == 'x')
          {
              use_caps = false;
          }
#if __OUTPUTF_ADVANCED_ENABLE
          if (flags_used & OUTPUTF_Flag_LengthLongLongInt)
          {
            uval = (uint64_t) va_arg(ap, uint64_t);
          }
          else
#endif
          {
            uval = (uint32_t) va_arg(ap, uint32_t);
          }
          vlen = radix_num_to_string(vstr, &uval, false, 16, use_caps);
          vstrp = &vstr[vlen];

#if __OUTPUTF_ADVANCED_ENABLE
          dschar = false;
          if (flags_used & OUTPUTF_Flag_Zero)
          {
            if (flags_used & OUTPUTF_Flag_Pound)
            {
              output_ch(output, '0');
              output_ch(output, (use_caps ? 'X' : 'x'));
              count += 2;
              // vlen += 2;
              dschar = true;
            }
            output_padding(output, '0', vlen, field_width, &count);
            vlen = field_width;
          }
          else
          {
            if (!(flags_used & OUTPUTF_Flag_Minus))
            {
              if (flags_used & OUTPUTF_Flag_Pound)
              {
                vlen += 2;
              }
              output_padding(output, ' ', vlen, field_width, &count);
              if (flags_used & OUTPUTF_Flag_Pound)
              {
                output_ch(output, '0');
                output_ch(output, use_caps ? 'X' : 'x');
                count += 2;

                dschar = true;
              }
            }
          }

          if ((flags_used & OUTPUTF_Flag_Pound) && (!dschar))
          {
            output_ch(output, '0');
            output_ch(output, use_caps ? 'X' : 'x');
            count += 2;
            vlen += 2;
          }
#endif
        }
        if ((c == 'o') || (c == 'b') || (c == 'p') || (c == 'u'))
        {
#if __OUTPUTF_ADVANCED_ENABLE
          if (flags_used & OUTPUTF_Flag_LengthLongLongInt)
          {
            uval = (uint64_t)va_arg(ap, uint64_t);
          }
          else
#endif
          {
            uval = (uint32_t)va_arg(ap, uint32_t);
          }
          switch (c)
          {
            case 'o':
              radix = 8;
              break;
            case 'b':
              radix = 2;
              break;
            case 'p':
              radix = 16;
              break;
            case 'u':
              radix = 10;
              break;
            default:
              break;
          }
          vlen = radix_num_to_string(vstr, &uval, false, radix, use_caps);
          vstrp = &vstr[vlen];
#if __OUTPUTF_ADVANCED_ENABLE
          if (flags_used & OUTPUTF_Flag_Zero)
          {
            output_padding(output, '0', vlen, field_width, &count);
            vlen = field_width;
          }
          else
          {
            if (!(flags_used & OUTPUTF_Flag_Minus))
            {
              output_padding(output, ' ', vlen, field_width, &count);
            }
          }
#endif
        }
#if !__OUTPUTF_ADVANCED_ENABLE
        output_padding(output, ' ', vlen, field_width, &count);
#endif
        if (vstrp != NULL)
        {
          while (*vstrp)
          {
            output_ch(output, *vstrp--);
            count++;
          }
        }
#if __OUTPUTF_ADVANCED_ENABLE
        if (flags_used & OUTPUTF_Flag_Minus)
        {
          output_padding(output, ' ', vlen, field_width, &count);
        }
#endif
      }
      else if (c == 'c')
      {
        cval = (char)va_arg(ap, uint32_t);
        output_ch(output, cval);
        count++;
      }
      else if (c == 's')
      {
        sval = (char *)va_arg(ap, char *);
        if (sval)
        {
#if __OUTPUTF_ADVANCED_ENABLE
          if (valid_precision_width)
          {
            vlen = precision_width;
          }
          else
          {
            vlen = strlen(sval);
          }
#else
          vlen = strlen(sval);
#endif
#if __OUTPUTF_ADVANCED_ENABLE
          if (!(flags_used & OUTPUTF_Flag_Minus))
#endif
          {
            output_padding(output, ' ', vlen, field_width, &count);
          }

#if __OUTPUTF_ADVANCED_ENABLE
          if (valid_precision_width)
          {
            while ((*sval) && (vlen > 0))
            {
              output_ch(output, *sval++);
              count++;
              vlen--;
            }
            // In case that vlen sval is shorter than vlen.
            vlen = precision_width - vlen;
          }
          else
          {
#endif
            while (*sval)
            {
              output_ch(output, *sval++);
              count++;
            }
#if __OUTPUTF_ADVANCED_ENABLE
          }
#endif

#if __OUTPUTF_ADVANCED_ENABLE
          if (flags_used & OUTPUTF_Flag_Minus)
          {
            output_padding(output, ' ', vlen, field_width, &count);
          }
#endif
        }
      }
      else
      {
        output_ch(output, c);
        count++;
      }
    }
  }
  return count;
}

// Converts an input line of ASCII characters based upon a provided
// string format.
static int scan(const char *line_ptr, const char *format, va_list args_ptr)
{
  int8_t neg;
  uint8_t base;
  int32_t val;
  uint32_t flag = 0;
  uint32_t field_width;
  uint32_t n_decode = 0;
  uint32_t n_assigned = 0;
  char temp;
  char *buf;
  const char *s;
  const char *c = format;
  const char *p = line_ptr;

  // Return EOF error before any conversion.
  if (*p == '\0')
  {
    return -1;
  }

  // Decode directives.
  while ((*c) && (*p))
  {
    // Ignore all white-spaces in the format strings.
    if (ignore_white_space((const char **) &c))
    {
      n_decode += ignore_white_space(&p);
    }
    else if ((*c != '%') || ((*c == '%') && (*(c + 1) == '%')))
    {
      // Ordinary characters.
      if (*p == *c)
      {
        if (*c == '%') c++;
        c++;
        p++;
        n_decode++;
      }
      else
      {
        // Match failure. Misalignment with C99, the unmatched characters
        // need to be pushed back to stream. However, it is deserted now.
        break;
      }
    }
    else
    {
      // Conversion specification.
      c++;
      // Reset.
      flag = 0;
      field_width = 0;
      base = 0;

      // Loop to get full conversion specification.
      while ((*c) && (!(flag & INPUTF_Flag_DestMask)))
      {
        switch (*c)
        {
#if __INPUTF_ADVANCED_ENABLE
          case '*':
            if (flag & INPUTF_Flag_Suppress)
            {
              // Match failure.
              return n_assigned;
            }
            flag |= INPUTF_Flag_Suppress;
            c++;
            break;
          case 'h':
            if (flag & INPUTF_Flag_LengthMask)
            {
              // Match failure.
              return n_assigned;
            }
            if (c[1] == 'h')
            {
              flag |= INPUTF_Flag_LengthChar;
              c++;
            }
            else
            {
              flag |= INPUTF_Flag_LengthShortInt;
            }
            c++;
            break;
          case 'l':
            if (flag & INPUTF_Flag_LengthMask)
            {
              // Match failure.
              return n_assigned;
            }
            if (c[1] == 'l')
            {
              flag |= INPUTF_Flag_LengthLongLongInt;
              c++;
            }
            else
            {
              flag |= INPUTF_Flag_LengthLongInt;
            }
            c++;
            break;
#endif
#if __INPUTF_FLOAT_ENABLE
          case 'L':
            if (flag & INPUTF_Flag_LengthMask)
            {
              // Match failure.
              return n_assigned;
            }
            flag |= INPUTF_Flag_LengthLongLongDouble;
            c++;
            break;
#endif
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if (field_width)
            {
              // Match failure.
              return n_assigned;
            }
            do
            {
              field_width = field_width * 10 + *c - '0';
              c++;
            } while ((*c >= '0') && (*c <= '9'));
            break;
          case 'd':
            base = 10;
            flag |= INPUTF_Flag_TypeSinged;
            flag |= INPUTF_Flag_DestInt;
            c++;
            break;
          case 'u':
            base = 10;
            flag |= INPUTF_Flag_DestInt;
            c++;
            break;
          case 'o':
            base = 8;
            flag |= INPUTF_Flag_DestInt;
            c++;
            break;
          case 'x':
          case 'X':
            base = 16;
            flag |= INPUTF_Flag_DestInt;
            c++;
            break;
          case 'i':
            base = 0;
            flag |= INPUTF_Flag_DestInt;
            c++;
            break;
#if __INPUTF_FLOAT_ENABLE
          case 'a':
          case 'A':
          case 'e':
          case 'E':
          case 'f':
          case 'F':
          case 'g':
          case 'G':
            flag |= INPUTF_Flag_DestFloat;
            c++;
            break;
#endif
          case 'c':
            flag |= INPUTF_Flag_DestChar;
            if (!field_width)
            {
              field_width = 1;
            }
            c++;
            break;
          case 's':
            flag |= INPUTF_Flag_DestString;
            c++;
            break;
          default:
            return n_assigned;
        }
      }

      if (!(flag & INPUTF_Flag_DestMask))
      {
        // Format strings are exhausted.
        return n_assigned;
      }

      if (!field_width)
      {
        // Larger than length of a line.
        field_width = 99;
      }

      // Matching strings in input streams and assign to argument.
      switch (flag & INPUTF_Flag_DestMask)
      {
        case INPUTF_Flag_DestChar:
          s = (const char *)p;
          buf = va_arg(args_ptr, char *);
          while ((field_width--) && (*p))
          {
            if (!(flag & INPUTF_Flag_Suppress))
            {
              *buf++ = *p++;
            }
            else
            {
              p++;
            }
            n_decode++;
          }
          if ((!(flag & INPUTF_Flag_Suppress)) && (s != p))
          {
            n_assigned++;
          }
          break;
        case INPUTF_Flag_DestString:
          n_decode += ignore_white_space(&p);
          s = p;
          buf = va_arg(args_ptr, char *);
          while ((field_width--) && (*p != '\0') && (*p != ' ') && (*p != '\t') && (*p != '\n') &&
                 (*p != '\r') && (*p != '\v') && (*p != '\f'))
          {
            if (flag & INPUTF_Flag_Suppress)
            {
              p++;
            }
            else
            {
              *buf++ = *p++;
            }
            n_decode++;
          }
          if ((!(flag & INPUTF_Flag_Suppress)) && (s != p))
          {
            // Add NULL to end of string.
            *buf = '\0';
            n_assigned++;
          }
          break;
        case INPUTF_Flag_DestInt:
            n_decode += ignore_white_space(&p);
            s = p;
            val = 0;
            if ((base == 0) || (base == 16))
            {
              if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))
              {
                base = 16;
                if (field_width >= 1)
                {
                  p += 2;
                  n_decode += 2;
                  field_width -= 2;
                }
              }
            }
            if (base == 0)
            {
              if (s[0] == '0')
              {
                base = 8;
              }
              else
              {
                base = 10;
              }
            }
            neg = 1;
            switch (*p)
            {
              case '-':
                neg = -1;
                n_decode++;
                p++;
                field_width--;
                break;
              case '+':
                neg = 1;
                n_decode++;
                p++;
                field_width--;
                break;
              default:
                break;
            }
            while ((*p) && (field_width--))
            {
              if ((*p <= '9') && (*p >= '0'))
              {
                temp = *p - '0';
              }
              else if ((*p <= 'f') && (*p >= 'a'))
              {
                temp = *p - 'a' + 10;
              }
              else if ((*p <= 'F') && (*p >= 'A'))
              {
                temp = *p - 'A' + 10;
              }
              else
              {
                temp = base;
              }
              if (temp >= base)
              {
                break;
              }
              else
              {
                val = base * val + temp;
              }
              p++;
              n_decode++;
            }
            val *= neg;
            if (!(flag & INPUTF_Flag_Suppress))
            {
#if __INPUTF_ADVANCED_ENABLE
              switch (flag & INPUTF_Flag_LengthMask)
              {
                case INPUTF_Flag_LengthChar:
                  if (flag & INPUTF_Flag_TypeSinged)
                  {
                    *va_arg(args_ptr, signed char *) = (signed char) val;
                  }
                  else
                  {
                    *va_arg(args_ptr, unsigned char *) = (unsigned char) val;
                  }
                  break;
                case INPUTF_Flag_LengthShortInt:
                  if (flag & INPUTF_Flag_TypeSinged)
                  {
                    *va_arg(args_ptr, signed short *) = (signed short) val;
                  }
                  else
                  {
                    *va_arg(args_ptr, unsigned short *) = (unsigned short) val;
                  }
                  break;
                case INPUTF_Flag_LengthLongInt:
                  if (flag & INPUTF_Flag_TypeSinged)
                  {
                    *va_arg(args_ptr, signed long int *) = (signed long int) val;
                  }
                  else
                  {
                    *va_arg(args_ptr, unsigned long int *) = (unsigned long int) val;
                  }
                  break;
                case INPUTF_Flag_LengthLongLongInt:
                  if (flag & INPUTF_Flag_TypeSinged)
                  {
                    *va_arg(args_ptr, signed long long int *) = (signed long long int) val;
                  }
                  else
                  {
                    *va_arg(args_ptr, unsigned long long int *) = (unsigned long long int) val;
                  }
                  break;
                default:
                  // The default type is the type int.
                  if (flag & INPUTF_Flag_TypeSinged)
                  {
                    *va_arg(args_ptr, signed int *) = (signed int) val;
                  }
                  else
                  {
                    *va_arg(args_ptr, unsigned int *) = (unsigned int) val;
                  }
                  break;
              }
#else
              // The default type is the type int.
              if (flag & INPUTF_Flag_TypeSinged)
              {
                *va_arg(args_ptr, signed int *) = (signed int) val;
              }
              else
              {
                *va_arg(args_ptr, unsigned int *) = (unsigned int) val;
              }
#endif
              n_assigned++;
            }
            break;
#if __INPUTF_FLOAT_ENABLE
        case INPUTF_Flag_DestFloat:
          n_decode += ignore_white_space(&p);
          fnum = strtod(p, (char **)&s);

          if ((fnum >= HUGE_VAL) || (fnum <= -HUGE_VAL))
          {
            break;
          }

          n_decode += (int)(s) - (int)(p);
          p = s;
          if (!(flag & INPUTF_Flag_Suppress))
          {
            if (flag & INPUTF_Flag_LengthLongLongDouble)
            {
              *va_arg(args_ptr, double *) = fnum;
            }
            else
            {
              *va_arg(args_ptr, float *) = (float)fnum;
            }
            n_assigned++;
          }
          break;
#endif
        default:
          return n_assigned;
      }
    }
  }
  return n_assigned;
}

int outputf(const char *fmt, ...)
{
  output_ch_t output;
  va_list args;

  // Initialize the output structure.  A null buffer
  // redirects output to the system serial console.
  output_init(&output, NULL, 0xffff, NULL);

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format function for the args.
  format(&output, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int voutputf(const char *fmt, va_list args)
{
  output_ch_t output;

  // Initialize the output structure.  A null buffer
  // redirects output to the system serial console.
  output_init(&output, NULL, 0xffff, NULL);

  // Call the format function for the args.
  format(&output, fmt, args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int soutputf(char *buffer, const char *fmt, ...)
{
  output_ch_t output;
  va_list args;

  // Initialize the output structure.
  output_init(&output, buffer, 0xffff, NULL);

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format function for the args.
  format(&output, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int snoutputf(char *buffer, size_t buflen, const char *fmt, ...)
{
  output_ch_t output;
  va_list args;

  // Sanity check buflen.
  if (buflen > 0xffff) return 0;

  // Initialize the output structure.
  output_init(&output, buffer, (uint16_t) buflen, NULL);

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format function for the args.
  format(&output, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int vsoutputf(char *buffer, const char *fmt, va_list args)
{
  output_ch_t output;

  // Initialize the output structure.
  output_init(&output, buffer, (uint16_t) 0xffff, NULL);

  // Call the format function for the args.
  format(&output, fmt, args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int vsnoutputf(char *buffer, size_t buflen, const char *fmt, va_list args)
{
  output_ch_t output;

  // Sanity check buflen.
  if (buflen > 0xffff) return 0;

  // Initialize the output structure.
  output_init(&output, buffer, (uint16_t) buflen, NULL);

  // Call the format function for the args.
  format(&output, fmt, args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int foutputf(putc_func_t func, const char *fmt, ...)
{
  output_ch_t output;
  va_list args;

  // Initialize the output structure.
  output_init(&output, NULL, 0xffff, func);

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format function for the args.
  format(&output, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int vfoutputf(putc_func_t func, const char *fmt, va_list args)
{
  output_ch_t output;

  // Initialize the output structure.
  output_init(&output, NULL, (uint16_t) 0xffff, func);

  // Call the format function for the args.
  format(&output, fmt, args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int faoutputf(putc_func_with_arg_t func, void *func_arg, const char *fmt, ...)
{
  output_ch_t output;
  va_list args;

  // Initialize the output structure.
  output_init_with_arg(&output, NULL, (uint16_t) 0xffff, func, func_arg);

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format function for the args.
  format(&output, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int vafoutputf(putc_func_with_arg_t func, void *func_arg, const char *fmt, va_list args)
{
  output_ch_t output;

  // Initialize the output structure.
  output_init_with_arg(&output, NULL, (uint16_t) 0xffff, func, func_arg);

  // Call the format function for the args.
  format(&output, fmt, args);

  // Finish output.
  output_end(&output);

  // Return the length of the string.
  return (int) output.index;
}

int sinputf(const char *buffer, const char *fmt, ...)
{
  int count;
  va_list args;

  // Initialize a variable argument list.
  va_start(args, fmt);

  // Call the format scanner.
  count = scan(buffer, fmt, args);

  // We are done with the argument list.
  va_end(args);

  // Return the number of items parsed.
  return count;
}

int vsinputf(const char *buffer, const char *fmt, va_list args)
{
  int count;

  // Call the format scanner.
  count = scan(buffer, fmt, args);

  // Return the number of items parsed.
  return count;
}



