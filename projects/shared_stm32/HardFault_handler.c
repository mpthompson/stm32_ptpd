/******************************************************************************
 * @file     HardFault_Handler.c
 * @brief    HardFault handler example
 * @version  V1.00
 * @date     10. July 2017
 ******************************************************************************/
/*
 * Copyright (c) 2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include "RTE_Components.h"             // Component selection
#include CMSIS_device_header            // Include device header file
#include "console.h"
#include "outputf.h"

#define HARDFAULT_HANDLER 1

#if HARDFAULT_HANDLER == 1

#if (defined (__GNUC__) || defined (__ARMCC_VERSION)) && !defined (__CC_ARM)
void MemManage_Handler_C(uint32_t *svc_args, uint32_t lr_value) __attribute__((used));
void MemManage_Handler(void) __attribute__((naked));

// MemManage handler wrapper in assembly language.
// It extracts the location of stack frame and passes it to the handler written
// in C as a pointer. We also extract the LR value as second parameter.
void MemManage_Handler(void)
{
  __asm volatile
  (
    " tst lr, #4                                                \n"
    " ite eq                                                    \n"
    " mrseq r0, msp                                             \n"
    " mrsne r0, psp                                             \n"
    " ldr r1, [r0, #24]                                         \n"
    " ldr r2, memmanage_address_const                           \n"
    " bx r2                                                     \n"
    " memmanage_address_const: .word MemManage_Handler_C        \n"
  );
}
#else
void MemManage_Handler_C(uint32_t *svc_args, uint32_t lr_value);

// MemManage handler wrapper in assembly language.
// It extracts the location of stack frame and passes it to the handler written
// in C as a pointer. We also extract the LR value as second parameter.
__asm void MemManage_Handler(void)
{
  TST    LR, #4
  ITE    EQ
  MRSEQ  R0, MSP
  MRSNE  R0, PSP
  MOV    R1, LR
  B      __cpp(MemManage_Handler_C)
}
#endif

// MemManage handler in C, with stack frame location and LR value extracted
// from the assembly wrapper as input parameters
void MemManage_Handler_C(uint32_t *memmanage_args, uint32_t lr_value)
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

  stacked_r0  = ((uint32_t) memmanage_args[0]);
  stacked_r1  = ((uint32_t) memmanage_args[1]);
  stacked_r2  = ((uint32_t) memmanage_args[2]);
  stacked_r3  = ((uint32_t) memmanage_args[3]);
  stacked_r12 = ((uint32_t) memmanage_args[4]);
  stacked_lr  = ((uint32_t) memmanage_args[5]);
  stacked_pc  = ((uint32_t) memmanage_args[6]);
  stacked_psr = ((uint32_t) memmanage_args[7]);

  foutputf(console_putc_interrupt, "[MemManage]\n");
  foutputf(console_putc_interrupt, "- Stack frame:\n"); 
  foutputf(console_putc_interrupt, " R0  = %x\n", stacked_r0);
  foutputf(console_putc_interrupt, " R1  = %x\n", stacked_r1);
  foutputf(console_putc_interrupt, " R2  = %x\n", stacked_r2);
  foutputf(console_putc_interrupt, " R3  = %x\n", stacked_r3);
  foutputf(console_putc_interrupt, " R12 = %x\n", stacked_r12);
  foutputf(console_putc_interrupt, " LR  = %x\n", stacked_lr);
  foutputf(console_putc_interrupt, " PC  = %x\n", stacked_pc);
  foutputf(console_putc_interrupt, " PSR = %x\n", stacked_psr);
  foutputf(console_putc_interrupt, "- FSR/FAR:\n");  
  foutputf(console_putc_interrupt, " CFSR = %x\n", cfsr);
  foutputf(console_putc_interrupt, " HFSR = %x\n", SCB->HFSR);
  foutputf(console_putc_interrupt, " DFSR = %x\n", SCB->DFSR);
  foutputf(console_putc_interrupt, " AFSR = %x\n", SCB->AFSR);
  if (cfsr & 0x0080) foutputf(console_putc_interrupt, " MMFAR = %x\n", memmanage_fault_address);
  if (cfsr & 0x8000) foutputf(console_putc_interrupt, " BFAR = %x\n", bus_fault_address);
  foutputf(console_putc_interrupt, "- Misc\n"); 
  foutputf(console_putc_interrupt, " LR/EXC_RETURN= %x\n", lr_value);

  __asm__("bkpt 0");

  while(1); // endless loop
}

#if (defined (__GNUC__) || defined (__ARMCC_VERSION)) && !defined (__CC_ARM)
void HardFault_Handler_C(uint32_t *svc_args, uint32_t lr_value) __attribute__((used));
void HardFault_Handler(void) __attribute__((naked));

// HardFault handler wrapper in assembly language.
// It extracts the location of stack frame and passes it to the handler written
// in C as a pointer. We also extract the LR value as second parameter.
void HardFault_Handler(void)
{
  __asm volatile
  (
    " tst lr, #4                                                \n"
    " ite eq                                                    \n"
    " mrseq r0, msp                                             \n"
    " mrsne r0, psp                                             \n"
    " ldr r1, [r0, #24]                                         \n"
    " ldr r2, hardfault_address_const                           \n"
    " bx r2                                                     \n"
    " hardfault_address_const: .word HardFault_Handler_C        \n"
  );
}

#else
void HardFault_Handler_C(uint32_t *svc_args, uint32_t lr_value);

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
#endif

// HardFault handler in C, with stack frame location and LR value extracted
// from the assembly wrapper as input parameters
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

  foutputf(console_putc_interrupt, "[HardFault]\n");
  foutputf(console_putc_interrupt, "- Stack frame:\n"); 
  foutputf(console_putc_interrupt, " R0  = %x\n", stacked_r0);
  foutputf(console_putc_interrupt, " R1  = %x\n", stacked_r1);
  foutputf(console_putc_interrupt, " R2  = %x\n", stacked_r2);
  foutputf(console_putc_interrupt, " R3  = %x\n", stacked_r3);
  foutputf(console_putc_interrupt, " R12 = %x\n", stacked_r12);
  foutputf(console_putc_interrupt, " LR  = %x\n", stacked_lr);
  foutputf(console_putc_interrupt, " PC  = %x\n", stacked_pc);
  foutputf(console_putc_interrupt, " PSR = %x\n", stacked_psr);
  foutputf(console_putc_interrupt, "- FSR/FAR:\n");  
  foutputf(console_putc_interrupt, " CFSR = %x\n", cfsr);
  foutputf(console_putc_interrupt, " HFSR = %x\n", SCB->HFSR);
  foutputf(console_putc_interrupt, " DFSR = %x\n", SCB->DFSR);
  foutputf(console_putc_interrupt, " AFSR = %x\n", SCB->AFSR);
  if (cfsr & 0x0080) foutputf(console_putc_interrupt, " MMFAR = %x\n", memmanage_fault_address);
  if (cfsr & 0x8000) foutputf(console_putc_interrupt, " BFAR = %x\n", bus_fault_address);
  foutputf(console_putc_interrupt, "- Misc\n"); 
  foutputf(console_putc_interrupt, " LR/EXC_RETURN= %x\n", lr_value);

  __asm__("bkpt 0");

  while(1); // endless loop
}

#endif
