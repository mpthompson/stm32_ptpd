#include <stdint.h>
#include "cmsis_os2.h"
#include "stm32f4xx.h"
#include "outputf.h"

// Cortex-M4 Processor Exceptions Handlers

#if defined(__CC_ARM)
// HardFault handler in C, with stack frame location and LR value extracted
// from the assembly wrapper as input parameters.  This is meant to be called
// by the assembly function that prepares the arguments.
void HardFault_Handler_C(uint32_t *hardfault_args, uint32_t lr_value)
{
  uint32_t stacked_r0;
  uint32_t stacked_r1;
  uint32_t stacked_r2;
  uint32_t stacked_r3;
  uint32_t stacked_r12;
  uint32_t stacked_lr;
  uint32_t stacked_pc;
  uint32_t stacked_psr;
  uint32_t cfsr;
  uint32_t bus_fault_address;
  uint32_t memmanage_fault_address;

  bus_fault_address       = SCB->BFAR;
  memmanage_fault_address = SCB->MMFAR;
  cfsr                    = SCB->CFSR;

  stacked_r0  = ((uint32_t) hardfault_args[0]);
  stacked_r1  = ((uint32_t) hardfault_args[1]);
  stacked_r2  = ((uint32_t) hardfault_args[2]);
  stacked_r3  = ((uint32_t) hardfault_args[3]);
  stacked_r12 = ((uint32_t) hardfault_args[4]);
  stacked_lr  = ((uint32_t) hardfault_args[5]);
  stacked_pc  = ((uint32_t) hardfault_args[6]);
  stacked_psr = ((uint32_t) hardfault_args[7]);

  outputf("[HardFault]\n");
  outputf("- Stack frame:\n"); 
  outputf(" R0  = %x\n", stacked_r0);
  outputf(" R1  = %x\n", stacked_r1);
  outputf(" R2  = %x\n", stacked_r2);
  outputf(" R3  = %x\n", stacked_r3);
  outputf(" R12 = %x\n", stacked_r12);
  outputf(" LR  = %x\n", stacked_lr);
  outputf(" PC  = %x\n", stacked_pc);
  outputf(" PSR = %x\n", stacked_psr);
  outputf("- FSR/FAR:\n");  
  outputf(" CFSR = %x\n", cfsr);
  outputf(" HFSR = %x\n", SCB->HFSR);
  outputf(" DFSR = %x\n", SCB->DFSR);
  outputf(" AFSR = %x\n", SCB->AFSR);
  if (cfsr & 0x0080) outputf(" MMFAR = %x\n", memmanage_fault_address);
  if (cfsr & 0x8000) outputf(" BFAR = %x\n", bus_fault_address);
  outputf("- Misc\n"); 
  outputf(" LR/EXC_RETURN= %x\n", lr_value);

  while(1); // endless loop
}

// HardFault handler wrapper in assembly language.
// It extracts the location of stack frame and passes it to the handler written
// in C as a pointer. We also extract the LR value as second parameter.
__asm void HardFault_Handler(void)
{
  TST    LR, #4
  ITE    EQ
  MRSEQ  R0, MSP
  MRSNE  R0, PSP
  MOV    R1, LR
  B      __cpp(HardFault_Handler_C)
}
#elif defined (__GNUC__)
// Handles Hard Fault exception.
void HardFault_Handler(void)
{
  // Go to infinite loop when Hard Fault exception occurs.
  while (1)
  {
  }
}
#endif

// Handles NMI exception.
void NMI_Handler(void)
{
}

// Handles Memory Manage exception.
void MemManage_Handler(void)
{
  // Go to infinite loop when Memory Manage exception occurs.
  while (1)
  {
  }
}

// Handles Bus Fault exception.
void BusFault_Handler(void)
{
  // Go to infinite loop when Bus Fault exception occurs.
  while (1)
  {
  }
}

// Handles Usage Fault exception.
void UsageFault_Handler(void)
{
  // Go to infinite loop when Usage Fault exception occurs.
  while (1)
  {
  }
}

// Handles Debug Monitor exception.
void DebugMon_Handler(void)
{
}

