#include <string.h>
#include <stdlib.h>
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "arch/sys_arch.h"
#include "outputf.h"

#if LWIP_SOCKET_SET_ERRNO
#ifndef errno
int errno = 0;
#endif
#endif

// System protection mutex.
static osMutexId_t sys_arch_mutex_id = NULL;

// Prints an assertion messages and aborts execution.
void sys_assert(char *pcMessage)
{
  // Output the message.
  outputf("%s\n", pcMessage);

  // XXX Stop system thread tasking.

  // Wait here forever.
  for (;;);
}

// Return a pseudo-random number.
u32_t lwip_rand(void)
{
  return (u32_t) rand();
}

// System initialization.
void sys_init(void)
{
  static uint32_t sys_arch_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));

  // Set the system mutex attributes. Note the mutex is not recursive.
  osMutexAttr_t sys_arch_mutex_attrs =
  {
    .name = "sys_arch",
    .attr_bits = 0,
    .cb_mem = sys_arch_mutex_cb,
    .cb_size = sizeof(sys_arch_mutex_cb)
  };

  // Create the system mutex.
  sys_arch_mutex_id = osMutexNew(&sys_arch_mutex_attrs);
  if (sys_arch_mutex_id == NULL)
  {
    outputf("sys_init error\n");
  }
}

// Returns kernel ticks since power up.
u32_t sys_jiffies(void)
{
  // Get the kernel tick count.
  return osKernelGetTickCount();
}

// Returns the system time in kernel ticks.
u32_t sys_now(void)
{
  // Get the kernel tick count.
  return osKernelGetTickCount();
}

// Does a "fast" critical region protection and returns the previous 
// protection level (not used here). This function should support 
// recursive calls from the same task or interrupt. In other words,
// sys_arch_protect() could be called while already protected. In that
// case the return value indicates that it is already protected.
sys_prot_t sys_arch_protect(void)
{
  if (osMutexAcquire(sys_arch_mutex_id, osWaitForever) != osOK)
  {
    outputf("sys_arch_protect error\n");
  }
  return (sys_prot_t) 1;
}

// This optional function does a "fast" set of critical region protection
// to the value specified by pval. See the documentation for
// sys_arch_protect() for more information.
void sys_arch_unprotect(sys_prot_t p)
{
  if (osMutexRelease(sys_arch_mutex_id) != osOK)
  {
    outputf("sys_arch_unprotect error\n");
  }
}

// Creates a new mailbox of queue_sz elements in the mailbox.
err_t sys_mbox_new(sys_mbox_t *mbox, int queue_sz)
{
  // We actually only deal with fixes size message queues.
  if (queue_sz > MB_SIZE)
  {
    outputf("sys_mbox_new size error\n");
    return ERR_MEM;
  }

  // Create the message queue for the mailbox.
  memset(mbox, 0, sizeof(*mbox));
  mbox->attr.cb_mem = mbox->data;
  mbox->attr.cb_size = sizeof(mbox->data);
  mbox->attr.mq_mem = mbox->queue;
  mbox->attr.mq_size = sizeof(mbox->queue);
  mbox->id = osMessageQueueNew((uint32_t) queue_sz, sizeof(void *), &mbox->attr);
  if (mbox->id == NULL)
  {
    outputf("sys_mbox_new create error\n");
    return ERR_MEM;
  }

  return ERR_OK;
}

// Deallocates a mailbox. If there are messages still present in the
// mailbox when the mailbox is deallocated, it is an indication of a
// programming error in lwIP and the developer should be notified.
void sys_mbox_free(sys_mbox_t *mbox)
{
  // Make sure we have the message queue.
  if (mbox->id)
  {
    // Report deleting a queue with items in it.
    if (osMessageQueueGetCount(mbox->id) > 0)
      outputf("sys_mbox_free error\n");
    // Delete the message queue and any internal resources.
    osMessageQueueDelete(mbox->id);
    mbox->id = NULL;
  }
}
 
// Post the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
  // Insert the "msg" into the message queue.
  osMessageQueuePut(mbox->id, &msg, 0, osWaitForever);
}
 
// Try to post the "msg" to the mailbox.  Returns immediately with
// ERR_MEM if cannot.
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  // Insert the "msg" into the message queue without a timeout.
  osStatus_t status = osMessageQueuePut(mbox->id, &msg, 0, 0);

  // Return ERR_MEM if the insertion failed.
  return status == osOK ? ERR_OK : ERR_MEM;
}

// Try to post a message to an mbox - may fail if full.
// To be be used from ISR. Returns ERR_MEM if it is full,
// else, ERR_OK if the "msg" is posted.
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
  // Insert the "msg" into the message queue without a timeout.
  osStatus_t status = osMessageQueuePut(mbox->id, &msg, 0, 0);

  // Return ERR_MEM if the insertion failed.
  return status == osOK ? ERR_OK : ERR_MEM;
}

// Blocks the thread until a message arrives in the mailbox, but does
// not block the thread longer than "timeout" milliseconds (similar to
// the sys_arch_sem_wait() function). The "msg" argument is a result
// parameter that is set by the function (i.e., by doing "*msg =
// ptr"). The "msg" parameter maybe NULL to indicate that the message
// should be dropped.
//
// The return values are the same as for the sys_arch_sem_wait() function:
// Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
// timeout.
//
// Note that a function with a similar name, sys_mbox_fetch(), is
// implemented by lwIP.
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
  void *message = NULL;

  // Get the start time.
  uint32_t start_time = osKernelGetTickCount();

  // A zero timeout means wait forever.
  if (timeout == 0) timeout = osWaitForever;

  // Get the "msg" from the message queue with the specified timeout.
  osStatus_t status = osMessageQueueGet(mbox->id, &message, 0, timeout);

  // Did we fail?
  if (status != osOK)
  {
    // Return a timeout.
    return SYS_ARCH_TIMEOUT;
  }

  // Make sure the message shouldn't be dropped.
  if (msg) *msg = message;

  // Return the time spent waiting for the message.
  return (u32_t) (osKernelGetTickCount() - start_time);
}

// Similar to sys_arch_mbox_fetch, but if message is not ready immediately,
// we'll return with SYS_MBOX_EMPTY.  On success, 0 is returned.
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  void *message = NULL;

  // Get the "msg" from the message queue with the specified timeout.
  osStatus_t status = osMessageQueueGet(mbox->id, &message, 0, 0);

  // Did we fail?
  if (status != osOK)
  {
    // Return empty.
    return SYS_MBOX_EMPTY;
  }

  // Make sure the message shouldn't be dropped.
  if (msg) *msg = message;

  return ERR_OK;
}

// Creates and returns a new semaphore. The "count" argument specifies
// the initial state of the semaphore.
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  // Create the semaphore.
  memset(sem, 0, sizeof(*sem));
  sem->attr.cb_mem = sem->data;
  sem->attr.cb_size = sizeof(sem->data);
  sem->id = osSemaphoreNew(UINT16_MAX, count, &sem->attr);
  if (sem->id == NULL)
  {
    outputf("sys_sem_new create error\n");
    return ERR_MEM;
  }

  return ERR_OK;
}

// Blocks the thread while waiting for the semaphore to be
// signaled. If the "timeout" argument is non-zero, the thread should
// only be blocked for the specified time (measured in
// milliseconds).
//
// If the timeout argument is non-zero, the return value is the number of
// milliseconds spent waiting for the semaphore to be signaled. If the
// semaphore wasn't signaled within the specified time, the return value is
// SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
// (i.e., it was already signaled), the function may return zero.
//
// Notice that lwIP implements a function with a similar name,
// sys_sem_wait(), that uses the sys_arch_sem_wait() function.
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  u32_t start = osKernelGetTickCount();

  // A zero timeout means wait forever.
  if (timeout == 0) timeout = osWaitForever;

  // Grab the semaphore.
  osStatus_t status = osSemaphoreAcquire(sem->id, timeout);

  // Did we fail?
  if (status != osOK)
  {
    return SYS_ARCH_TIMEOUT;
  }

  // Return the time spent waiting to grab the semaphore.
  return (u32_t) (osKernelGetTickCount() - start);
}

// Signals (releases) a semaphore
void sys_sem_signal(sys_sem_t *data)
{
  if (osSemaphoreRelease(data->id) != osOK)
    for(;;); // Can be called by ISR do not use printf.
}

// Deallocates a semaphore
void sys_sem_free(sys_sem_t *sem)
{
  // Make sure we have the semaphore.
  if (sem->id)
  {
    // Delete the semaphore and any internal resources.
    osSemaphoreDelete(sem->id);
    sem->id = NULL;
  }
}

// Create a new mutex.
err_t sys_mutex_new(sys_mutex_t *mutex)
{
  static uint32_t sys_mutex_index = 0;
  static uint32_t sys_mutex_cb[4][osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static char *sys_mutex_names[4] = { "tcpip_mem", "tcpip_core", "lwip_mutex_1", "lwip_mutex_2" };

  // Creating mutexes within the array above helps with debugging. We allow up to four mutexes
  // to be created this way before creating them outside the array.
  memset(mutex, 0, sizeof(*mutex));
  if (sys_mutex_index < 4)
  {
    mutex->attr.name = sys_mutex_names[sys_mutex_index];
    mutex->attr.attr_bits = osMutexRecursive | osMutexPrioInherit;
    mutex->attr.cb_mem = &sys_mutex_cb[sys_mutex_index];
    mutex->attr.cb_size = sizeof(sys_mutex_cb[sys_mutex_index]);
    sys_mutex_index += 1;
  }
  else
  {
    mutex->attr.name = "lwip_mutex";
    mutex->attr.attr_bits = osMutexRecursive | osMutexPrioInherit;
    mutex->attr.cb_mem = mutex->data;
    mutex->attr.cb_size = sizeof(mutex->data);
  }
  mutex->id = osMutexNew(&mutex->attr);
  if (mutex->id == NULL)
  {
    outputf("sys_mutex_new create error\n");
    return ERR_MEM;
  }

  return ERR_OK;
}

// Lock a mutex.
void sys_mutex_lock(sys_mutex_t *mutex)
{
  if (osMutexAcquire(mutex->id, osWaitForever) != osOK)
    outputf("sys_mutex_lock error\n");
}
 
// Unlock a mutex.
void sys_mutex_unlock(sys_mutex_t *mutex)
{
  if (osMutexRelease(mutex->id) != osOK)
    outputf("sys_mutex_unlock error\n");
}
 
// Deallocate a mutex.
void sys_mutex_free(sys_mutex_t *mutex)
{
  // Make sure we have the mutex.
  if (mutex->id)
  {
    // Delete the mutex and any internal resources.
    osMutexDelete(mutex->id);
    mutex->id = NULL;
  }
}

// Keep a pool of thread structures
static int thread_pool_index = 0;
static sys_thread_data_t thread_pool[SYS_THREAD_POOL_N];
 
// Starts a new thread with priority "prio" that will begin its
// execution in the function "thread()". The "arg" argument will be
// passed as an argument to the thread() function. The id of the new
// thread is returned. Both the id and the priority are system
// dependent.
sys_thread_t sys_thread_new(const char *name, void (*thread)(void *arg),
                            void *arg, int stacksize, int priority)
{
  LWIP_DEBUGF(SYS_DEBUG, ("New Thread: %s\n", name));

  if (thread_pool_index >= SYS_THREAD_POOL_N)
    outputf("sys_thread_new number error\n");

  sys_thread_t t = (sys_thread_t)&thread_pool[thread_pool_index];
  thread_pool_index++;

  memset(t, 0, sizeof(*t));
  t->attr.name = name ? name : "lwip_unnamed_thread";
  t->attr.priority = (osPriority_t) priority;
  // XXX t->attr.cb_size = sizeof(t->data);
  // XXX t->attr.cb_mem = t->data;
  t->attr.cb_size = 0;
  t->attr.cb_mem = NULL;
  t->attr.stack_size = stacksize;
  t->attr.stack_mem = NULL;
  t->id = osThreadNew((osThreadFunc_t) thread, arg, &t->attr);
  if (t->id == NULL)
    outputf("sys_thread_new create error\n");

  return t;
}


