#include <string.h>
#include "cmsis_os2.h"
#include "hal_system.h"
#include "extint.h"

// Array of callback functions.
static extint_func_with_arg_t extint_callbacks[EXTINT_COUNT];

// Array of callback arguments.
static void *extint_callback_args[EXTINT_COUNT];

// Array to assist with index to line values.
static const uint32_t extint_lines[EXTINT_COUNT] =
{
  LL_EXTI_LINE_0,
  LL_EXTI_LINE_1,
  LL_EXTI_LINE_2,
  LL_EXTI_LINE_3,
  LL_EXTI_LINE_4,
  LL_EXTI_LINE_5,
  LL_EXTI_LINE_6,
  LL_EXTI_LINE_7,
  LL_EXTI_LINE_8,
  LL_EXTI_LINE_9,
  LL_EXTI_LINE_10,
  LL_EXTI_LINE_11,
  LL_EXTI_LINE_12,
  LL_EXTI_LINE_13,
  LL_EXTI_LINE_14,
  LL_EXTI_LINE_15
};

__STATIC_INLINE void extint_handler(uint32_t index)
{
  // Get the external interrupt line index.
  uint32_t extint_line = extint_lines[index];

  // Make sure that interrupt flag is set.
  if (EXTI->PR & extint_line)
  {
    // Clear interrupt flag.
    EXTI->PR = extint_line;

    // Call the callback function pointer with the callback argument.
    if (extint_callbacks[index] != NULL) extint_callbacks[index](extint_line, extint_callback_args[index]);
  }
}

// External interrupt line 0 handler.
void EXTI0_IRQHandler(void)
{
  extint_handler(EXTINT_0);
}

// External interrupt line 1 handler.
void EXTI1_IRQHandler(void)
{
  extint_handler(EXTINT_1);
}

// External interrupt line 2 handler.
void EXTI2_IRQHandler(void)
{
  extint_handler(EXTINT_2);
}

// External interrupt line 3 handler.
void EXTI3_IRQHandler(void)
{
  extint_handler(EXTINT_3);
}

// External interrupt line 3 handler.
void EXTI4_IRQHandler(void)
{
  extint_handler(EXTINT_4);
}

// External interrupt line 5-9 handler.
void EXTI9_5_IRQHandler(void)
{
  extint_handler(EXTINT_5);
  extint_handler(EXTINT_6);
  extint_handler(EXTINT_7);
  extint_handler(EXTINT_8);
  extint_handler(EXTINT_9);
};

// External interrupt line 10-15 handler.
void EXTI15_10_IRQHandler(void)
{
  extint_handler(EXTINT_10);
  extint_handler(EXTINT_11);
  extint_handler(EXTINT_12);
  extint_handler(EXTINT_13);
  extint_handler(EXTINT_14);
  extint_handler(EXTINT_15);
};

// Initialize external interrupt handling.
// WARNING: This function called prior to RTOS initialization.
void extint_init(void)
{
  uint32_t grouping;
  uint32_t priority;
  uint32_t interrupt_preempt_priority = extint_config_preempt_priority();

  // Clear out all the function pointers.
  memset(extint_callbacks, 0, sizeof(extint_callbacks));

  // Get the NVIC priority grouping.
  grouping = NVIC_GetPriorityGrouping();

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI0_IRQn, priority);
  NVIC_EnableIRQ(EXTI0_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI1_IRQn, priority);
  NVIC_EnableIRQ(EXTI1_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI2_IRQn, priority);
  NVIC_EnableIRQ(EXTI2_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI3_IRQn, priority);
  NVIC_EnableIRQ(EXTI3_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI4_IRQn, priority);
  NVIC_EnableIRQ(EXTI4_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI9_5_IRQn, priority);
  NVIC_EnableIRQ(EXTI9_5_IRQn);

  // Enable the external interrupts.
  priority = NVIC_EncodePriority(grouping, interrupt_preempt_priority, 0);
  NVIC_SetPriority(EXTI15_10_IRQn, priority);
  NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// Set an external interrupt callback function.
void extint_set_callback(uint32_t line, extint_func_t callback)
{
  // Set a callback with a null argument.
  extint_set_callback_with_arg(line, (extint_func_with_arg_t) callback, NULL);
}

// Set an external interrupt callback function with callback argument.
void extint_set_callback_with_arg(uint32_t line, extint_func_with_arg_t callback, void *arg)
{
  uint32_t index;

  // Find the index of the corresponding line.
  for (index = 0; index < EXTINT_COUNT; ++index)
  {
    // Break from the loop if we found the index of the line.
    if (extint_lines[index] == line) break;
  }

  // Did we find the index?
  if (index < EXTINT_COUNT)
  {
    // Yes. Set the line callback function and callback argument.
    extint_callbacks[index] = callback;
    extint_callback_args[index] = arg;
  }
}

// System configurable external preemptive interrupt priority value.
__WEAK uint32_t extint_config_preempt_priority(void)
{
  return 5;
}

