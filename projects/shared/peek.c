#include <string.h>
#include "cmsis_os2.h"
#include "shell.h"
#include "outputf.h"
#include "peek.h"

// Peek shell function.
static bool peek_shell_peek(int argc, char **argv)
{
  bool is_peek = true;
  bool is_word = false;
  bool is_long = false;
  bool needs_help = false;
  uint32_t i;
  uint32_t value;
  uint32_t address = 0;
  uint32_t count = 1;

  // Determine if this is peek or poke?
  if (!strncasecmp("poke", argv[0], 4)) is_peek = false;

  // Is this a word or long word?
  if (!strncasecmp("w", argv[0] + 4, 1)) is_word = true;
  if (!strncasecmp("l", argv[0] + 4, 1)) is_long = true;

  // Do we have peek or poke?
  if (is_peek && (argc > 1))
  {
    // Parse the address.
    if (sinputf(argv[1], "%i", &address) == 1)
    {
      // Parse the count if it is available.
      if (argc > 2) sinputf(argv[2], "%i", &count);

      // Keep the count reasonable.
      if (count < 1) count = 1;
      if (count > 100) count = 100;

      // Peek each value.
      for (i = 0; i < count; ++i)
      {
        // Read the value.
        if (is_long)
        {
          address &= 0xfffffffc;
          value = *((uint32_t *) address);
        }
        else if (is_word)
        {
          address &= 0xfffffffe;
          value = (uint32_t) *((uint16_t *) address);
        }
        else
        {
          value = (uint32_t) *((uint8_t *) address);
        }

        // Print the value and increment the address.
        if (is_long)
        {
          shell_printf("0x%08x: 0x%08x\n", address, value);
          address += 4;
        }
        else if (is_word)
        {
          shell_printf("0x%08x: 0x%04x\n", address, value & 0xffff);
          address += 2;
        }
        else
        {
          shell_printf("0x%08x: 0x%02x\n", address, value & 0xff);
          address += 1;
        }
      }
    }
    else
    {
      // We need help.
      needs_help = true;
    }
  }
  else if (!is_peek && (argc > 2))
  {
    // Parse the address.
    if (sinputf(argv[1], "%i", &address) == 1)
    {
      // Parse the the value.
      if (sinputf(argv[2], "%i", &value) == 1)
      {
        // Parse the count if it is available.
        if (argc > 3) sinputf(argv[3], "%i", &count);

        // Keep the count reasonable.
        if (count < 1) count = 1;
        if (count > 100) count = 100;

        // Poke each value.
        for (i = 0; i < count; ++i)
        {
          // Write the value then read it immediately.
          if (is_long)
          {
            address &= 0xfffffffc;
            *((uint32_t *) address) = value;
            value = *((uint32_t *) address);
          }
          else if (is_word)
          {
            address &= 0xfffffffe;
            *((uint16_t *) address) = (uint16_t) (value & 0xffff);
            value = (uint32_t) *((uint16_t *) address);
          }
          else
          {
            *((uint8_t *) address) = (uint8_t) (value & 0xff);
            value = (uint32_t) *((uint8_t *) address);
          }

          // Print the value and increment the address.
          if (is_long)
          {
            shell_printf("0x%08x: 0x%08x\n", address, value);
            address += 4;
          }
          else if (is_word)
          {
            shell_printf("0x%08x: 0x%04x\n", address, value & 0xffff);
            address += 2;
          }
          else
          {
            shell_printf("0x%08x: 0x%02x\n", address, value & 0xff);
            address += 1;
          }
        }
      }
      else
      {
        // We need help.
        needs_help = true;
      }
    }
    else
    {
      // We need help.
      needs_help = true;
    }
  }
  else
  {
    // We need help.
    needs_help = true;
  }

  // Do we need help?
  if (needs_help)
  {
    shell_printf("Usage:\n");
    shell_printf("  peekb address [count]\n");
    shell_printf("  peekw address [count]\n");
    shell_printf("  peekl address [count]\n");
    shell_printf("  pokeb address value [count]\n");
    shell_printf("  pokew address value [count]\n");
    shell_printf("  pokel address value [count]\n");
  }

  return true;
}

// Initialize the peek and poke memory functions.
void peek_init(void)
{
  // Add the peek and poke shell commands.
  shell_add_command("peekb", peek_shell_peek);
  shell_add_command("peekw", peek_shell_peek);
  shell_add_command("peekl", peek_shell_peek);
  shell_add_command("pokeb", peek_shell_peek);
  shell_add_command("pokew", peek_shell_peek);
  shell_add_command("pokel", peek_shell_peek);
}
