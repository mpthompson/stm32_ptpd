#include "cmsis_os2.h"
#include "ll_drivers.h"
#include "exceptions.h"

// Cortex-M4 Processor Exception Handlers

// This function handles NMI exception.
void NMI_Handler(void)
{
}

// This function handles Hard Fault exception.
void HardFault_Handler(void)
{
  // Go to infinite loop when Hard Fault exception occurs.
  for (;;);
}

// This function handles Memory Manage exception.
void MemManage_Handler(void)
{
  // Go to infinite loop when Memory Manage exception occurs.
  for (;;);
}

// This function handles Bus Fault exception.
void BusFault_Handler(void)
{
  // Go to infinite loop when Bus Fault exception occurs.
  for (;;);
}

// This function handles Usage Fault exception.
void UsageFault_Handler(void)
{
  // Go to infinite loop when Usage Fault exception occurs.
  for (;;);
}

// This function handles Debug Monitor exception.
void DebugMon_Handler(void)
{
}
