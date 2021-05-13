#include "cmsis_os2.h"
#include "rtx_lib.h"
#include "tick.h"
#include "shell.h"
#include CMSIS_device_header

#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST;

// Load average calculation.
static uint32_t idle_count = 0U;
static uint32_t total_count = 0U;
static uint32_t last_idle_count = 0U;
static uint32_t last_total_count = 0U;

// Global tick frequency. Modified to actual value in OS_Tick_Setup().
uint32_t tick_frequency = 1000U;

// Setup OS Tick. The timer should be configured to generate periodic interrupts at
// frequency specified by freq. The parameter handler defines the interrupt handler
// function that is called. The timer should only be initialized and configured, but
// must not be started to create interrupts. The RTOS kernel calls the function 
// OS_Tick_Enable() to start the timer interrupts.
int32_t OS_Tick_Setup(uint32_t freq, IRQHandler_t handler)
{
  uint32_t load;
  (void)handler;

  if (freq == 0U) return -1;
  tick_frequency = freq;

  load = (SystemCoreClock / freq) - 1U;
  if (load > 0x00FFFFFFU) return -1;

  // Set SysTick Interrupt Priority
#if   ((defined(__ARM_ARCH_8M_MAIN__) && (__ARM_ARCH_8M_MAIN__ != 0)) || \
       (defined(__CORTEX_M)           && (__CORTEX_M           == 7U)))
  SCB->SHPR[11] = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_8M_BASE__) && (__ARM_ARCH_8M_BASE__ != 0))
  SCB->SHPR[1] |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#elif ((defined(__ARM_ARCH_7M__)      && (__ARM_ARCH_7M__      != 0)) || \
       (defined(__ARM_ARCH_7EM__)     && (__ARM_ARCH_7EM__     != 0)))
  SCB->SHP[11]  = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_6M__)      && (__ARM_ARCH_6M__      != 0))
  SCB->SHP[1]  |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#else
#error "Unknown ARM Core!"
#endif

  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
  SysTick->LOAD = load;
  SysTick->VAL  = 0U;

  PendST = 0U;

  return 0;
}

// Enable OS Tick. Enable and start the OS Tick timer to generate periodic RTOS Kernel Tick interrupts.
void OS_Tick_Enable(void)
{
  if (PendST != 0U)
  {
    PendST = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
  }

  SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}

// Disable OS Tick. Stop the OS Tick timer and disable generation of RTOS Kernel Tick interrupts.
void OS_Tick_Disable(void)
{
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

  if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U)
  {
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ. Acknowledge the execution of the OS Tick timer interrupt
// function, for example clear the pending flag.
void OS_Tick_AcknowledgeIRQ(void)
{
  // Has the count exceeded the frequency?
  if (total_count >= tick_frequency)
  {
    // Save the current counts.
    last_idle_count = idle_count;
    last_total_count = total_count;

    // Reset the current counts.
    idle_count = 0U;
    total_count = 0U;
  }

  // Increment the total count and idle counts.
  total_count++;
  if (osRtxThreadGetRunning() == osRtxInfo.thread.idle) idle_count++;

  (void) SysTick->CTRL;
}

// Get OS Tick IRQ number. Return the numeric value that identifies the interrupt called
// by the OS Tick timer.
int32_t OS_Tick_GetIRQn(void)
{
  return (int32_t) SysTick_IRQn;
}

// Get OS Tick clock. Return the input clock frequency of the OS Tick timer. This is the
// increment rate of the counter value returned by the function OS_Tick_GetCount().
uint32_t OS_Tick_GetClock(void)
{
  return SystemCoreClock;
}

// Get OS Tick interval. Return the number of counter ticks between to periodic 
// OS Tick timer interrupts.
uint32_t OS_Tick_GetInterval(void)
{
  return (SysTick->LOAD + 1U);
}

// Get OS Tick count value. Return the current value of the OS Tick counter: 0 ... (reload value -1).
uint32_t OS_Tick_GetCount(void)
{
  uint32_t load = SysTick->LOAD;
  return (load - SysTick->VAL);
}

// Get OS Tick overflow status. Returns the state of OS Tick timer interrupt pending bit that
// indicates timer overflows to adjust SysTimer calculations.
uint32_t OS_Tick_GetOverflow(void)
{
  return ((SysTick->CTRL >> 16) & 1U);
}

// Tick shell utility.
static bool tick_shell_command(int argc, char **argv)
{
  // Get the load average as a percentage.
  uint32_t percent_idle = tick_get_percent_idle();

  // Print the load as 100 - percent_idle.
  shell_printf("Load Average: %u%%\n", 100 - percent_idle);

  return true;
}

// Initialize tick shell command.
void tick_init(void)
{
  // Set the load average shell command.
  shell_add_command("load", tick_shell_command);
}

// Returns the percent of time over the previous second the system is idle.
uint32_t tick_get_percent_idle(void)
{
  uint32_t percent_idle = 100;

  // Disable the tick interrupt.
  OS_Tick_Disable();

  // Determine the percent time the system is idle.
  if (last_total_count > 0) percent_idle = (last_idle_count * 100u) / last_total_count;
  if (percent_idle > 100) percent_idle = 100;

  // Enable the tick interrupt.
  OS_Tick_Enable();

  return percent_idle;
}
