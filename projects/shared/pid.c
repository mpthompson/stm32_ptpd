#include <string.h>
#include "cmsis_os2.h"
#include "pid.h"

// Initialize the PID state structure.
void pid_init_state(pid_state_t *state, float input, float p_gain, float d_gain, float i_gain)
{
  // Initialize state structure.
  memset(state, 0, sizeof(pid_state_t));
  state->p_gain = p_gain;
  state->d_gain = d_gain;
  state->i_gain = i_gain;
  state->prev_input = input;
}

// Set limits to the PID state structure.
void pid_set_limits(pid_state_t *state, float max_input, float max_control, float max_integrator, float max_output)
{
  // Set the limits.
  state->max_input = max_input;
  state->min_input = -max_input;
  state->max_control = max_control;
  state->min_control = -max_control;
  state->max_integrator = max_integrator;
  state->min_integrator = -max_integrator;
  state->max_output = max_output;
  state->min_output = -max_output;
}

// Set maximum limits to the PID state structure.
void pid_set_max_limits(pid_state_t *state, float max_input, float max_control, float max_integrator, float max_output)
{
  // Set the maximum limits.
  state->max_input = max_input;
  state->max_control = max_control;
  state->max_integrator = max_integrator;
  state->max_output = max_output;
}

// Set minimum limits to the PID state structure.
void pid_set_min_limits(pid_state_t *state, float min_input, float min_control, float min_integrator, float min_output)
{
  // Set the minimum limits.
  state->min_input = min_input;
  state->min_control = min_control;
  state->min_integrator = min_integrator;
  state->min_output = min_output;
}

// Reset the PID integrator state.
void pid_reset_integrator(pid_state_t *state)
{
  // Reset just the integrator state.
  state->integrator = 0.0f;
}

// Reset the PID state structure.
void pid_reset_state(pid_state_t *state, float input)
{
  // Reset the state structure.
  state->integrator = 0.0f;
  state->prev_input = input;
}

// Perform a PID calculation producing the output value from the input and state.
float pid_calculate(pid_state_t *state, float input, float control)
{
  float error;
  float velocity;
  float p_term;
  float d_term;
  float i_term;
  float output;

  // Limit the input and control values.
  if (input > state->max_input)
    input = state->max_input;
  else if (input < state->min_input)
    input = state->min_input;
  if (control > state->max_control)
    control = state->max_control;
  else if (control < state->min_control)
    control = state->min_control;

  // Determine the error.
  error = control - input;

  // Determine the velocity.
  velocity = input - state->prev_input;

  // Update the input for determining velocity.
  state->prev_input = input;

  // Update the integrator state. Note the integrator state includes
  // the integrator gain so that integrator limits determines how much
  // of the output can be driven by the integrator state.
  state->integrator += error * state->i_gain;

  // Limit the integrator state if necessary
  if (state->integrator > state->max_integrator)
    state->integrator = state->max_integrator;
  else if (state->integrator < state->min_integrator)
    state->integrator = state->min_integrator;

  // Deterine the proportional term.
  p_term = error * state->p_gain;

  // Determine the integrator term.
  i_term = state->integrator;

  // Determine the derivative term.
  d_term = velocity * state->d_gain;

  // Determine the output by summing up the P-term, I-term and D-term.
  // Notice the D-term is a dampening factor so it is subtracted.
  output = p_term + i_term - d_term;

  // Add the optional feed forward constant.
  output += state->feed_forward;

  // Limit the output.
  if (output > state->max_output)
    output = state->max_output;
  else if (output < state->min_output)
    output = state->min_output;

  return output;
}
