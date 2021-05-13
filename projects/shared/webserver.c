#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "syslog.h"
#include "stm32f4xx.h"
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "webserver.h"

static const char test_page[] = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Connection: Close\r\n\r\n"
                        "<html>\n"
                        "<head>\n"
                        "<title>Web Server Test Page</title>\n"
                        "<style type='text/css'>"
                        "body{font-family:'Arial, sans-serif', sans-serif;font-size:.8em;background-color:#fff;}"
                        "</style>\n"
                        "</head>\n"
                        "<body>\n"
                        "<h1>Congratulations!</h1>\n"
                        "<p>If you can see this page, it is working properly.</p>\n"
                        "</body>\n"
                        "</html>\n";
 
static err_t webserver_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  char *data;

  // Check if status is ok and data is arrived.
  if (err == ERR_OK && p != NULL)
  {
    // Inform TCP that we have taken the data.
    tcp_recved(pcb, p->tot_len);

    // Point to the incoming data.
    data = (char *) p->payload;

    // If the data is a GET request we can handle it.
    if (strncmp(data, "GET ", 4) == 0)
    {
      // Write the data to the system.
      if (tcp_write(pcb, (void *) test_page, strlen(test_page), 1) == ERR_OK)
      {
        tcp_output(pcb);
        tcp_close(pcb);
      }
    }
    else
    {
      // Non-GET request.
    }
  }
  else
  {
    // No data arrived. That means the client closes the connection and
    // sent us a packet with FIN flag set to 1. We have to cleanup and
    // destroy our TCP connection.
  }

  // Free the buffer passed in.
  if (p != NULL) pbuf_free(p);

  // We succeeded.
  return ERR_OK;
}

// Accept an incomming call on the registered port.
static err_t webserver_accept_callback(void *arg, struct tcp_pcb *npcb, err_t err)
{
  (void)(arg);

  // Subscribe a receive callback function.
  tcp_recv(npcb, &webserver_recv_callback);

  // We succeeded.
  return ERR_OK;
}

// Initialize within the context of the tcpip thread.
void webserver_init_callback(void *arg)
{
  struct tcp_pcb *pcb;

  // Bind a function to a tcp port.
  if ((pcb = tcp_new()) != NULL)
  {
    // Bind to port 80.
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) == ERR_OK)
    {
      // Create a listing endpoint.
      if ((pcb = tcp_listen(pcb)) != NULL)
      {
        // Set accept callback.
        tcp_accept(pcb, &webserver_accept_callback);
      }
    }
  }
}

// Initialize the web server.
void webserver_init(void)
{
  // We need to do this within the context of the tcpip thread.
  tcpip_callback(webserver_init_callback, NULL);
}
