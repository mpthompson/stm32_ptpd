#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

// This file breaks abstraction layers and uses RTX internal types, but it limits the 
// contamination to single, RTOS implementation specific, header file, therefore limiting
// scope of possible changes.

#include "lwip/err.h"
#include "cmsis_os2.h"
#include "rtx_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

void sys_assert(char *msg);
u32_t lwip_rand(void);

// === SEMAPHORE ===
typedef struct
{
  osSemaphoreId_t id;
  osSemaphoreAttr_t attr;
  uint32_t data[osRtxSemaphoreCbSize/4U];
} sys_sem_t;

#define sys_sem_valid(x)        (((*x).id == NULL) ? 0 : 1)
#define sys_sem_set_invalid(x)  ( (*x).id = NULL)

// === MUTEX ===
typedef struct
{
  osMutexId_t id;
  osMutexAttr_t attr;
  uint32_t data[osRtxMutexCbSize/4U];
} sys_mutex_t;

// === MAIL BOX ===
#define MB_SIZE               32

typedef struct
{
  osMessageQueueId_t id;
  osMessageQueueAttr_t attr;
  uint32_t data[osRtxMessageQueueCbSize/4U];
  uint32_t queue[osRtxMessageQueueMemSize(MB_SIZE, sizeof(void *))/4U];
} sys_mbox_t;

#define SYS_MBOX_NULL               ((uint32_t) NULL)
#define sys_mbox_valid(x)           (((*x).id == NULL) ? 0 : 1)
#define sys_mbox_set_invalid(x)     ( (*x).id = NULL)

#if ((DEFAULT_RAW_RECVMBOX_SIZE) > (MB_SIZE)) || \
    ((DEFAULT_UDP_RECVMBOX_SIZE) > (MB_SIZE)) || \
    ((DEFAULT_TCP_RECVMBOX_SIZE) > (MB_SIZE)) || \
    ((DEFAULT_ACCEPTMBOX_SIZE)   > (MB_SIZE)) || \
    ((TCPIP_MBOX_SIZE)           > (MB_SIZE))
#   error Mailbox size not supported
#endif

// === THREAD ===
typedef struct
{
  osThreadId_t id;
  osThreadAttr_t attr;
  uint32_t data[osRtxThreadCbSize/4U];
} sys_thread_data_t;
typedef sys_thread_data_t* sys_thread_t;

#define SYS_THREAD_POOL_N                   6
#define SYS_DEFAULT_THREAD_STACK_DEPTH      DEFAULT_STACK_SIZE

// === PROTECTION ===
typedef int sys_prot_t;

// === TIMER ===
typedef struct
{
  osTimerId_t id;
  osTimerAttr_t attr;
  uint32_t data[osRtxTimerCbSize/4U];
} sys_timer_t;

#ifdef __cplusplus
}
#endif

#endif /* __SYS_ARCH_H__ */

