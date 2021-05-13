#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "syslog.h"
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "shell.h"
#include "webclient.h"
#include "outputf.h"

// See: https://www.ntu.edu.sg/home/ehchua/programming/webprogramming/HTTP_Basics.html

// This may need to be adjusted to better fit requested data.
#define HEADER_SIZE    64

// Client parse states.
enum
{
  WEBCLIENT_PARSE_STATUS = 0,
  WEBCLIENT_PARSE_RESPONSE,
  WEBCLIENT_PARSE_BODY
};

// Client state information.
typedef struct webclient_state_s
{
  bool busy;
  ip4_addr_t addr;
  uint16_t port;
  webclient_func_t func;
  uint32_t timeout;
  uint32_t parse_state;
  uint32_t final_status;
  uint32_t request_len;
  uint32_t status_len;
  uint32_t response_len;
  char request[HEADER_SIZE];
  char status[HEADER_SIZE];
  char response[HEADER_SIZE];
} webclient_state_t;

// State of the connection.
static webclient_state_t webclient_state;

// Parse out the next non-space word from a string.
// str    Pointer to pointer to the string
// word   Pointer to pointer of next word.
// Returns 0:Failed, 1:Successful
static int webclient_parse_field(char **str, char **field)
{
  // Skip leading spaces.
  while (**str && isspace((unsigned char) **str)) (*str)++;

  // Set the field.
  *field = *str;

  // Skip non-space characters.
  while (**str && !isspace((unsigned char) **str)) (*str)++;

  // Null terminate the field.
  if (**str) *(*str)++ = 0;

  return (*str != *field) ? 1 : 0;
}

// Parse the HTTP status line to extract the status code.
static int webclient_parse_status(char *status)
{
  int i;
  int argc = 0;
  char *argv[4];
  int status_code = 0;

#ifdef __GNUC__
// GNUC includes strncasecmp() in strings.h, not string.h.  We
// define the external function here to simplify cross compilation
// between ARMCC and GNUC.
extern int strncasecmp(const char *, const char *, size_t n);
#endif

  // XXX outputf("WEBCLIENT: status='%s'\n", status);

  // Parse the status line into space deliminated fields.
  for (i = 0; i < (sizeof(argv) / sizeof(char *)); ++i)
  {
    webclient_parse_field(&status, &argv[i]);
    if (*argv[i] != 0) ++argc;
  }

  // Does the status line make sense?
  if ((argc > 1) && (strncasecmp(argv[0], "HTTP/1.", 7) == 0))
  {
    // Get the status code as an integer.
    status_code = atoi(argv[1]);
  }

  // We return the numeric status code.  The value should
  // be between 200 and 299 to indicate success.
  return status_code;
}

// Parse the HTTP response line.
static void webclient_parse_response(char *response)
{
  // Ignore the response lines for now.
  // In the future we may wish to parse Content-Type,
  // Content-Length or other response values.
  // XXX outputf("WEBCLIENT: response='%s'\n", response);
}

// Web client function to parse received data.
static err_t webclient_parse_data(webclient_state_t *state, const uint8_t *buffer, uint32_t buflen)
{
  // Assume we succeed.
  err_t status = ERR_OK;

  // Parse each character.
  while (buflen)
  {
    // Handle the parse state.
    switch (state->parse_state)
    {
      case WEBCLIENT_PARSE_STATUS:
        // Gather the status line up until the EOL character.
        if (*buffer == '\n')
        {
          int status_code;

          // Null terminate the status line.
          state->status[state->status_len] = 0;

          // Parse the status code from the status line.
          status_code = webclient_parse_status(state->status);

          // Does the HTTP status code indicate success?
          if ((status_code >= 200) && (status_code < 300))
          {
            // Yes. Parse the response headers.
            state->response_len = 0;
            state->parse_state = WEBCLIENT_PARSE_RESPONSE;
          }
          else
          {
            // No. We failed so return error.
            status = ERR_ABRT;

            // Adjust the buffer length to terminate parsing this buffer..
            buffer += buflen - 1;
            buflen = 1;
          }
        }
        else if ((*buffer >= ' ') && (*buffer <= '~'))
        {
          // Insert into the status buffer being careful of overflow.
          if (state->status_len < (HEADER_SIZE - 1))
          {
            // Insert the character into status buffer.
            state->status[state->status_len] = (char) *buffer;
            state->status_len += 1;
          }
        }
        break;

      case WEBCLIENT_PARSE_RESPONSE:
        // Gather the response line up until the EOL character.
        if (*buffer == '\n')
        {
          // Was this an empty line?
          if (state->response_len == 0)
          {
            // Yes. Move on to parsing the body data.
            state->parse_state = WEBCLIENT_PARSE_BODY;

            // Update the final status to OK as we are parsing data.
            state->final_status = WEBCLIENT_STS_OK;
          }
          else
          {
            // Null terminate the response line.
            state->response[state->response_len] = 0;

            // Parse the response line.
            webclient_parse_response(state->response);

            // Reset the response line length.
            state->response_len = 0;
          }
        }
        else if ((*buffer >= ' ') && (*buffer <= '~'))
        {
          // Insert into the response buffer being careful of overflow.
          if (state->response_len < (HEADER_SIZE - 1))
          {
            // Insert the character into status buffer.
            state->response[state->response_len] = (char) *buffer;
            state->response_len += 1;
          }
        }
        break;

      case WEBCLIENT_PARSE_BODY:
        // Pass the body data through to the callback function.
        if (state->func) state->func(WEBCLIENT_STS_OK, buffer, buflen);

        // Adjust the buffer length to terminate parsing this buffer..
        buffer += buflen - 1;
        buflen = 1;

        break;
    }

    // Increment the buffer and decrement the length.
    buffer += 1;
    buflen -= 1;
  }

  // Update the final status.
  state->final_status = status == ERR_OK ? WEBCLIENT_STS_OK : WEBCLIENT_STS_NOTFOUND;
  
  return status;
}

// Close the PCB.
static err_t webclient_close_pcb(struct tcp_pcb *pcb)
{
  // Sanity check the PCB.
  if (pcb == NULL) return ERR_OK;

  // Clear everything out for this connection.
  tcp_poll(pcb, NULL, 0);
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
  tcp_err(pcb, NULL);
  tcp_arg(pcb, NULL);

  // Close the TCP connection
  return tcp_close(pcb);
}

// Web client receive callback.
static err_t webclient_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  uint8_t *data;
  uint32_t datalen;
  webclient_state_t *state = (webclient_state_t *) arg;

  // Reset the timeout.
  state->timeout = 0;

  // Check if status is ok and data is arrived.
  if (err == ERR_OK && p != NULL)
  {
    // Inform TCP that we have taken the data.
    tcp_recved(pcb, p->tot_len);

    // Point to the incoming data.
    data = (uint8_t *) p->payload;
    datalen = (uint32_t) p->tot_len;

    // Parse the incoming data.
    if (webclient_parse_data(state, data, datalen) != ERR_OK)
    {
      // Parsing indicated some type of error.
      if (state->func) state->func(state->final_status, NULL, 0);

      // Clear the state.
      memset(state, 0, sizeof(webclient_state_t));

      // Close the TCP connection
      webclient_close_pcb(pcb);
    }
  }
  else
  {
    // No data arrived. That means the client closes the connection and
    // sent us a packet with FIN flag set to 1.
    if (state->func) state->func(state->final_status, NULL, 0);

    // Clear the state.
    memset(state, 0, sizeof(webclient_state_t));

    // Close the TCP connection.
    webclient_close_pcb(pcb);
  }

  // Free the buffer passed in.
  if (p != NULL) pbuf_free(p);

  // We succeeded.
  return ERR_OK;
}

// Web client sent callback.
static err_t webclient_sent_callback(void *arg, struct tcp_pcb *pcb, uint16_t len)
{
  webclient_state_t *state = (webclient_state_t *) arg;

  // Reset the timeout.
  state->timeout = 0;

  return ERR_OK;
}

// Web client poll callback.
static err_t webclient_poll_callback(void *arg, struct tcp_pcb *pcb)
{
  webclient_state_t *state = (webclient_state_t *) arg;

  // Increment the timeout.
  state->timeout += 1;

  // Have we exceeded four seconds?
  if (state->timeout >= 4)
  {
    // Close the TCP connection
    webclient_close_pcb(pcb);

    // We experienced a timeout.
    if (state->func) state->func(WEBCLIENT_STS_TIMEOUT, NULL, 0);

    // Clear the state.
    memset(state, 0, sizeof(webclient_state_t));
  }

  return ERR_OK;
}

// Web client error callback.
static void webclient_err_callback(void *arg, err_t err)
{
  webclient_state_t *state = (webclient_state_t *) arg;

  // We failed to connect.
  if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

  // Clear the state.
  memset(state, 0, sizeof(webclient_state_t));
}

// Web client connected callback.
static err_t webclient_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err)
{
  webclient_state_t *state = (webclient_state_t *) arg;

  // Was the connection successful?
  if (err == ERR_OK)
  {
    // Update the state final status.  We assume the server doesn't have the
    // resource until we parse the status line.
    state->final_status = WEBCLIENT_STS_NOTFOUND;

    // Send the HTTP to retrieve the desired item from the server.
    if ((tcp_write(pcb, state->request, state->request_len, 0) != ERR_OK) || (tcp_output(pcb) != ERR_OK))
    {
      // Close the TCP connection
      webclient_close_pcb(pcb);

      // We failed in the connection.
      if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

      // Clear the state.
      memset(state, 0, sizeof(webclient_state_t));
    }
  }
  else
  {
    // Close the TCP connection
    webclient_close_pcb(pcb);

    // We failed in the connection.
    if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

    // Clear the state.
    memset(state, 0, sizeof(webclient_state_t));
  }

  return ERR_OK;
}

// Initiate a connection to the given web server.
void webclient_get_callback(void *arg)
{
  struct tcp_pcb *pcb;
  webclient_state_t *state = (webclient_state_t *) arg;

  // Create a new TCP protocol buffer.
  if ((pcb = tcp_new()) != NULL)
  {
    // Bind to any address and any port.
    if (tcp_bind(pcb, IP_ADDR_ANY, 0) == ERR_OK)
    {
      // Set the connection structure.
      tcp_arg(pcb, state);

      // Set receive/sent/error callbacks.
      tcp_recv(pcb, &webclient_recv_callback);
      tcp_sent(pcb, &webclient_sent_callback);
      tcp_err(pcb, &webclient_err_callback);

      // Request a polling callback so we can implement timeouts. The interval
      // is specified in number of TCP coarse grained timer shots, which typically
      // occurs twice a second. An interval of 2 means that the application
      // would be polled every 1 second.
      tcp_poll(pcb, webclient_poll_callback, 2);

      // Open the connection to the server.
      if (tcp_connect(pcb, &state->addr, state->port, webclient_connected_callback) != ERR_OK)
      {
        // We failed to connect.
        if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

        // Clear the state.
        memset(state, 0, sizeof(webclient_state_t));
      }
    }
    else
    {
      // We failed to bind.
      if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

      // Clear the state.
      memset(state, 0, sizeof(webclient_state_t));
    }
  }
  else
  {
    // We failed to allocate a new TCP protocol buffer.
    if (state->func) state->func(WEBCLIENT_STS_ERROR, NULL, 0);

    // Clear the state.
    memset(state, 0, sizeof(webclient_state_t));
  }
}

// Callback from shell command.
static void webclient_shell_callback(uint32_t status, const uint8_t *buffer, uint32_t buflen)
{
  // Is the status OK?
  if (status == WEBCLIENT_STS_OK)
  {
    if (!buflen)
    {
      // End of data.
      // XXX outputf("WEBCLIENT: end of data\n");
    }
    else
    {
      // Loop over the buffered data.
      while (buflen)
      {
        // Put the data out the serial port.
        outputf("%c", (char) *buffer);

        // Increment the buffer and decrement the length.
        buffer += 1;
        buflen -= 1;
      }
    }
  }
  else
  {
    // We got some status other than OK.
    outputf("WEBCLIENT: status=%u\n", status);
  }
}

// Shell command to test the webclient.
static bool webclient_shell_www(int argc, char **argv)
{
  ip4_addr_t request_server;
  uint16_t request_port;
  char *request_path;
  bool help = false;

  // Do we have the correct number of arguments?
  if (argc == 4)
  {
    // Convert the arguments to server address, port and request path.
    ip4addr_aton(argv[1], &request_server);
    request_port = (uint16_t) atoi(argv[2]);
    request_path = argv[3];

    // Issue the get request using the specified information.
    webclient_get(request_server, request_port, request_path, webclient_shell_callback);
  }
  else
  {
    // We need help.
    help = true;
  }

  if (help)
  {
    // Give usage.
    shell_printf("Usage: www server port path\n");
  }

  return true;
}

// Initialize HTTP client module.
void webclient_init(void)
{
  // Clear out the webclient state.
  memset(&webclient_state, 0, sizeof(webclient_state_t));

  // Add the shell command.
  shell_add_command("www", webclient_shell_www);
}

// Intiate an HTTP GET operation to the indicated server, port and path.  Data
// associated with the get will be returned in the callback function.
int webclient_get(ip4_addr_t addr, uint16_t port, const char *path, webclient_func_t func)
{
  // Return if the client is already busy.
  if (webclient_state.busy) return -1;

  // Initialize the client structure.
  memset(&webclient_state, 0, sizeof(webclient_state_t));
  webclient_state.addr = addr;
  webclient_state.port = port;
  webclient_state.func = func;
  webclient_state.parse_state = WEBCLIENT_PARSE_STATUS;

  // Assume we failed to process the request until proven otherwise.
  webclient_state.final_status = WEBCLIENT_STS_ERROR;

  // Fill in the GET request with the requested path.
  webclient_state.request_len = snoutputf(webclient_state.request, HEADER_SIZE, "GET /%s HTTP/1.0\r\n\r\n", path);

  // Mark the client as busy.
  webclient_state.busy = true;

  // Initiate the connection within the context of the tcpip thread.
  if (tcpip_callback(webclient_get_callback, &webclient_state) != ERR_OK)
  {
    // We failed are are no longer busy.
    memset(&webclient_state, 0, sizeof(webclient_state_t));

    // Return error.
    return -1;
  }

  // We succeeded.
  return 0;
}

