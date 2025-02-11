/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2008 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file querystring_example.c
 * @brief example for how to get the query string from libmicrohttpd
 *        Call with an URI ending with something like "?q=QUERY"
 * @author Christian Grothoff
 */

#include "platform.h"
#include <microhttpd.h>

#define PAGE \
  "<html><head><title>libmicrohttpd demo</title></head><body>Query string for &quot;%s&quot; was &quot;%s&quot;</body></html>"

static enum MHD_Result
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **req_cls)
{
  static int aptr;
  const char *fmt = cls;
  const char *val;
  char *me;
  struct MHD_Response *response;
  enum MHD_Result ret;
  int resp_len;
  (void) url;               /* Unused. Silent compiler warning. */
  (void) version;           /* Unused. Silent compiler warning. */
  (void) upload_data;       /* Unused. Silent compiler warning. */
  (void) upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *req_cls)
  {
    /* do never respond on first call */
    *req_cls = &aptr;
    return MHD_YES;
  }
  *req_cls = NULL;  /* reset when done */
  if (NULL == fmt)
    return MHD_NO;  /* The cls must not be NULL */
  val = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "q");
  if (NULL == val)
    return MHD_NO;  /* No "q" argument was found */
  resp_len = snprintf (NULL, 0, fmt, "q", val);
  if (0 > resp_len)
    return MHD_NO;  /* Error calculating response size */
  me = malloc (resp_len + 1);
  if (me == NULL)
    return MHD_NO;  /* Error allocating memory */
  if (resp_len != snprintf (me, resp_len + 1, fmt, "q", val))
  {
    free (me);
    return MHD_NO;  /* Error forming the response body */
  }
  response = MHD_create_response_from_buffer (resp_len, me,
                                              MHD_RESPMEM_MUST_FREE);
  if (response == NULL)
  {
    free (me);
    return MHD_NO;
  }
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  int port;

  if (argc != 2)
  {
    printf ("%s PORT\n", argv[0]);
    return 1;
  }
  port = atoi (argv[1]);
  if ( (port < 0) ||
       (port > UINT16_MAX) )
  {
    printf ("%s PORT\n", argv[0]);
    return 1;
  }
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION
                        | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        (uint16_t) port,
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  if (NULL == d)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
