#ifndef __PID_H__
#define __PID_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PID state structure.
typedef struct pid_state_s
{
  // Gains.
  float p_gain;
  float d_gain;
  float i_gain;

  // Feed forward.
  float feed_forward;

  // Config values.
  float max_input;
  float min_input;
  float max_control;
  float min_control;
  float max_integrator;
  float min_integrator;
  float max_output;
  float min_output;

  // State values.
  float integrator;
  float prev_input;
} pid_state_t;

void pid_init_state(pid_state_t *state, float input, float p_gain, float d_gain, float i_gain);
void pid_set_limits(pid_state_t *state, float max_input, float max_control, float max_integrator, float max_output);
void pid_set_max_limits(pid_state_t *state, float max_input, float max_control, float max_integrator, float max_output);
void pid_set_min_limits(pid_state_t *state, float min_input, float min_control, float min_integrator, float min_output);
void pid_reset_integrator(pid_state_t *state);
void pid_reset_state(pid_state_t *state, float input);
float pid_calculate(pid_state_t *state, float input, float control);

// Set PID p-gain.
static inline void pid_set_p_gain(pid_state_t *state, float gain)
{
  state->p_gain = gain;
}

// Get PID p-gain.
static inline float pid_get_p_gain(pid_state_t *state)
{
  return state->p_gain;
}

// Set PID d-gain.
static inline void pid_set_d_gain(pid_state_t *state, float gain)
{
  state->d_gain = gain;
}

// Get PID d-gain.
static inline float pid_get_d_gain(pid_state_t *state)
{
  return state->d_gain;
}

// Set PID i-gain.
static inline void pid_set_i_gain(pid_state_t *state, float gain)
{
  state->i_gain = gain;
}

// Get PID i-gain.
static inline float pid_get_i_gain(pid_state_t *state)
{
  return state->i_gain;
}

// Set PID feed forward.
static inline void pid_set_feed_forward(pid_state_t *state, float feed_forward)
{
  state->feed_forward = feed_forward;
}

// Get PID feed forward.
static inline float pid_get_feed_forward(pid_state_t *state)
{
  return state->feed_forward;
}

#ifdef __cplusplus
}
#endif

#endif // __PID_H__
