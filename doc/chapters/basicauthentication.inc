With the small exception of IP address based access control,
requests from all connecting clients where served equally until now.
This chapter discusses a first method of client's authentication and
its limits.

A very simple approach feasible with the means already discussed would
be to expect the password in the @emph{URI} string before granting access to
the secured areas. The password could be separated from the actual resource identifier
by a certain character, thus the request line might look like
@verbatim
GET /picture.png?mypassword
@end verbatim
@noindent

In the rare situation where the client is customized enough and the connection
occurs through secured lines (e.g., a embedded device directly attached to
another via wire) and where the ability to embed a password in the URI or to
pass on a URI with a password are desired, this can be a reasonable choice.

But when it is assumed that the user connecting does so with an ordinary
Internet browser, this implementation brings some problems about. For example,
the URI including the password stays in the address field or at least in the
history of the browser for anybody near enough to see.  It will also be
inconvenient to add the password manually to any new URI when the browser does
not know how to compose this automatically.

At least the convenience issue can be addressed by employing the simplest
built-in password facilities of HTTP compliant browsers, hence we want to
start there. It will, however, turn out to have still severe weaknesses in
terms of security which need consideration.

Before we will start implementing @emph{Basic Authentication} as described in
@emph{RFC 2617}, we will also abandon the simplistic and generally
problematic practice of responding every request the first time our callback
is called for a given connection. Queuing a response upon the first request
is akin to generating an error response (even if it is a "200 OK" reply!).
The reason is that MHD usually calls the callback in three phases:

@enumerate
@item
First, to initially tell the application about the connection and inquire whether
it is OK to proceed. This call typically happens before the client could upload
the request body, and can be used to tell the client to not proceed with the
upload (if the client requested "Expect: 100 Continue"). Applications may queue
a reply at this point, but it will force the connection to be closed and thus
prevent keep-alive / pipelining, which is generally a bad idea. Applications
wanting to proceed with the request throughout the other phases should just return
"MHD_YES" and not queue any response.  Note that when an application suspends
a connection in this callback, the phase does not advance and the application
will be called again in this first phase.
@item
Next, to tell the application about upload data provided by the client.
In this phase, the application may not queue replies, and trying to do so
will result in MHD returning an error code from @code{MHD_queue_response}.
If there is no upload data, this phase is skipped.
@item
Finally, to obtain a regular response from the application. This can be
almost any type of response, including ones indicating failures. The
one exception is a "100 Continue" response, which applications must never
generate: MHD generates that response automatically when necessary in the
first phase.  If the application does not queue a response, MHD may call
the callback repeatedly (depending a bit on the threading model, the
application should suspend the connection).
@end enumerate

But how can we tell whether the callback has been called before for the
particular request?  Initially, the pointer this parameter references is
set by @emph{MHD} in the callback. But it will also be "remembered" on the
next call (for the same request).  Thus, we can use the @code{req_cls}
location to keep track of the request state.  For now, we will simply
generate no response until the parameter is non-null---implying the callback
was called before at least once. We do not need to share information between
different calls of the callback, so we can set the parameter to any address
that is assured to be not null. The pointer to the @code{connection} structure
will be pointing to a legal address, so we take this.

The first time @code{answer_to_connection} is called, we will not even look at the headers.

@verbatim
static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method, const char *version,
                      const char *upload_data, size_t *upload_data_size,
                      void **req_cls)
{
  if (0 != strcmp(method, "GET")) return MHD_NO;
  if (NULL == *req_cls) {*req_cls = connection; return MHD_YES;}

  ...
  /* else respond accordingly */
  ...
}
@end verbatim
@noindent

Note how we lop off the connection on the first condition (no "GET" request),
but return asking for more on the other one with @code{MHD_YES}.  With this
minor change, we can proceed to implement the actual authentication process.

@heading Request for authentication

Let us assume we had only files not intended to be handed out without the
correct username/password, so every "GET" request will be challenged.
@emph{RFC 2617} describes how the server shall ask for authentication by
adding a @emph{WWW-Authenticate} response header with the name of the
@emph{realm} protected.  MHD can generate and queue such a failure response
for you using the @code{MHD_queue_basic_auth_fail_response} API.  The only
thing you need to do is construct a response with the error page to be shown
to the user if he aborts basic authentication.  But first, you should check if
the proper credentials were already supplied using the
@code{MHD_basic_auth_get_username_password} call.

Your code would then look like this:
@verbatim
static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **req_cls)
{
  char *user;
  char *pass;
  int fail;
  enum MHD_Result ret;
  struct MHD_Response *response;

  if (0 != strcmp (method, MHD_HTTP_METHOD_GET))
    return MHD_NO;
  if (NULL == *req_cls)
    {
      *req_cls = connection;
      return MHD_YES;
    }
  pass = NULL;
  user = MHD_basic_auth_get_username_password (connection, &pass);
  fail = ( (user == NULL) ||
	   (0 != strcmp (user, "root")) ||
	   (0 != strcmp (pass, "pa$$w0rd") ) );
  if (user != NULL) free (user);
  if (pass != NULL) free (pass);
  if (fail)
    {
      const char *page = "<html><body>Go away.</body></html>";
      response =
	MHD_create_response_from_buffer (strlen (page), (void *) page,
				       MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_basic_auth_fail_response (connection,
						"my realm",
						response);
    }
  else
    {
      const char *page = "<html><body>A secret.</body></html>";
      response =
	MHD_create_response_from_buffer (strlen (page), (void *) page,
				       MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    }
  MHD_destroy_response (response);
  return ret;
}
@end verbatim

See the @code{examples} directory for the complete example file.

@heading Remarks
For a proper server, the conditional statements leading to a return of @code{MHD_NO} should yield a
response with a more precise status code instead of silently closing the connection. For example,
failures of memory allocation are best reported as @emph{internal server error} and unexpected
authentication methods as @emph{400 bad request}.

@heading Exercises
@itemize @bullet
@item
Make the server respond to wrong credentials (but otherwise well-formed requests) with the recommended
@emph{401 unauthorized} status code. If the client still does not authenticate correctly within the
same connection, close it and store the client's IP address for a certain time. (It is OK to check for
expiration not until the main thread wakes up again on the next connection.) If the client fails
authenticating three times during this period, add it to another list for which the
@code{AcceptPolicyCallback} function denies connection (temporally).

@item
With the network utility @code{netcat} connect and log the response of a "GET" request as you
did in the exercise of the first example, this time to a file. Now stop the server and let @emph{netcat}
listen on the same port the server used to listen on and have it fake being the proper server by giving
the file's content as the response (e.g. @code{cat log | nc -l -p 8888}). Pretending to think your were
connecting to the actual server, browse to the eavesdropper and give the correct credentials.

Copy and paste the encoded string you see in @code{netcat}'s output to some of the Base64 decode tools available online
and see how both the user's name and password could be completely restored.

@end itemize
