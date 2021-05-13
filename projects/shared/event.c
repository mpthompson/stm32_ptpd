#include <stdbool.h>
#include "cmsis_os2.h"
#include "rtx_lib.h"
#include "console.h"
#include "event.h"
#include "tick.h"

#define EVENT_TIMER_COUNT       10u
#define EVENT_QUEUE_COUNT       16u

// Event message queue element structure.
typedef struct event_message_s
{
  event_t handler;
  void *arg;
} event_message_t;

// Event 1 Hz callbacks.
static event_message_t event_1hz_callbacks[EVENT_TIMER_COUNT];

// Event 1 Hz mutex.
static osMutexId_t event_1hz_mutex_id = NULL;

// Event 1 Hz timer.
static osTimerId_t event_1hz_timer_id = NULL;

// Event message queue.
static osMessageQueueId_t event_msg_queue_id = NULL;

// Callback for the 1 Hz event timer. This is called on the timer thread which has
// limited resources so we schedule an event for the actual callback. This
// callback is actually called at a higher rate than 1 Hz, but it will call each
// timer callback at a 1 Hz rate. We go though this mechanism to spread all the
// callbacks evenly over the course of 1 second.
static void event_1hz_timer_callback(void *arg)
{
  (void) arg;

  static uint32_t count = 0u;

  // Increment the count to cycle between each callback element.
  count = (count + 1) % EVENT_TIMER_COUNT;

  // Lock the 1 Hz mutex. Notice we ignore this timer callback if we can't
  // lock the mutex as we want to avoid blocking the timer thread.
  if (osMutexAcquire(event_1hz_mutex_id, 0) == osOK)
  {
    // Is there an event handler associated with this timer count.
    if (event_1hz_callbacks[count].handler != NULL)
    {
      // Yes. Schedule an event callback with the argument.
      event_schedule(event_1hz_callbacks[count].handler, event_1hz_callbacks[count].arg, 0);
    }

    // Unlock the 1 Hz mutex.
    osMutexRelease(event_1hz_mutex_id);
  }
}

// Initialize the event infrastructure.
void event_init(void)
{
  // Static message queue control block.
  static uint32_t message_queue_cb[osRtxMessageQueueCbSize/4U] __attribute__((section(".bss.os.msgqueue.cb")));

  // Static message queue element memory.
  static uint32_t queue_mem[osRtxMessageQueueMemSize(EVENT_QUEUE_COUNT, sizeof(event_message_t))/4U] __attribute__((section(".bss.os.msgqueue.mem")));

  // Static mutex and timer control blocks.
  static uint32_t event_1hz_mutex_cb[osRtxMutexCbSize/4U] __attribute__((section(".bss.os.mutex.cb")));
  static uint32_t event_1hz_timer_cb[osRtxTimerCbSize/4U] __attribute__((section(".bss.os.timer.cb")));

  // Initialize the message queue attributes.
  osMessageQueueAttr_t message_queue_attrs =
  {
    .name = "event",
    .attr_bits = 0U,
    .cb_mem = message_queue_cb,
    .cb_size = sizeof(message_queue_cb),
    .mq_mem = queue_mem,
    .mq_size = sizeof(queue_mem)
  };

  // Configure 1Hz mutex attributes. Note the mutex is not recursive.
  osMutexAttr_t event_1hz_mutex_attrs =
  {
    .name = "event_1hz",
    .attr_bits = 0,
    .cb_mem = event_1hz_mutex_cb,
    .cb_size = sizeof(event_1hz_mutex_cb)
  };

  // Configure 1Hz timer attributes.
  osTimerAttr_t event_1hz_timer_attrs =
  {
    .name = "event_1hz",
    .attr_bits = 0U,
    .cb_mem = event_1hz_timer_cb,
    .cb_size = sizeof(event_1hz_timer_cb)
  };

  // Create the event message queue.
  event_msg_queue_id = osMessageQueueNew(EVENT_QUEUE_COUNT, sizeof(event_message_t), &message_queue_attrs);
  if (!event_msg_queue_id)
  {
    // Event message queue not created, handle failure.
    console_puts("EVENT: cannot create message queue\n");
  }

  // Create the event 1 Hz mutex.
  event_1hz_mutex_id = osMutexNew(&event_1hz_mutex_attrs);
  if (event_1hz_mutex_id == NULL)
  {
    // Event 1 Hz mutex not created, handle failure.
    console_puts("EVENT: cannot create 1Hz mutex\n");
  }

  // Create the event 1 Hz timer.
  event_1hz_timer_id = osTimerNew(event_1hz_timer_callback, osTimerPeriodic, NULL, &event_1hz_timer_attrs);
  if (event_1hz_timer_id == NULL)
  {
    // Event 1 Hz mutex not created, handle failure.
    console_puts("EVENT: cannot create 1Hz timer\n");
  }

  // Clear out the array of 1 Hz event callbacks.
  memset(event_1hz_callbacks, 0, sizeof(event_1hz_callbacks));
}

// Event processing loop.
void event_loop(void)
{
  osStatus_t status;
  event_message_t event_message;

  // Do we have a 1 Hz callback timer?
  if (event_1hz_timer_id)
  {
    // Start the 1 Hz callback timer. The actual timer is set to cycle through each
    // timer callback at 1 Hz rate so it must be divided by the number of callbacks.
    osTimerStart(event_1hz_timer_id, tick_from_milliseconds(1000u / EVENT_TIMER_COUNT));
  }

  // Loop forever.
  for (;;)
  {
    // Wait for next event to be queued.
    status = osMessageQueueGet(event_msg_queue_id, &event_message, NULL, osWaitForever);

    // Did we receive an event?
    if (status == osOK)
    {
      // Call the event handler with the arg.
      event_message.handler(event_message.arg);
    }
  }
}

// Schedule an event to be processed with a timeout in milliseconds.
// The timeout must be zero if called from an interrupt context.
int event_schedule(event_t handler, void *arg, uint32_t timeout)
{
  // Assume we failed.
  osStatus_t status = osErrorResource;

  // Make sure we are initialized.
  if (event_msg_queue_id)
  {
    event_message_t event_message;

    // Fill in the event message.
    event_message.handler = handler;
    event_message.arg = arg;

    // Send the event message.
    status = osMessageQueuePut(event_msg_queue_id, &event_message, 0, tick_from_milliseconds(timeout));
  }

  // Return 0 if OK, or -1 if an error.
  return status == osOK ? 0 : -1;
}

// Schedule a repeating 1 Hz event with argument to be processed. Typically used
// for periodic monitoring of a module.
int event_schedule_1hz(event_t handler, void *arg)
{
  int status = -1;

  // Lock the 1 Hz mutex.
  osMutexAcquire(event_1hz_mutex_id, osWaitForever);

  // Loop over each callback slot.
  for (uint32_t i = 0; (status != 0) && (i < EVENT_TIMER_COUNT); ++i)
  {
    // Is this callback slot open?
    if (event_1hz_callbacks[i].handler == NULL)
    {
      // Fill in this callback slot.
      event_1hz_callbacks[i].handler = handler;
      event_1hz_callbacks[i].arg = arg;

      // We succeeded.
      status = 0;
    }
  }

  // Unlock the 1 Hz mutex.
  osMutexRelease(event_1hz_mutex_id);

  return status;
}

// Cancel a repeating 1hz event.
int event_cancel_1hz(event_t handler)
{
  int status = -1;

  // Lock the 1 Hz mutex.
  osMutexAcquire(event_1hz_mutex_id, osWaitForever);

  // Loop over each callback slot.
  for (uint32_t i = 0; i < EVENT_TIMER_COUNT; ++i)
  {
    // Does this slot match the callback?
    if (event_1hz_callbacks[i].handler == handler)
    {
      // Reset this callback slot.
      event_1hz_callbacks[i].handler = (event_t) NULL;
      event_1hz_callbacks[i].arg = NULL;

      // We succeeded.
      status = 0;
    }
  }

  // Unlock the 1 Hz mutex.
  osMutexRelease(event_1hz_mutex_id);

  return status;
}
