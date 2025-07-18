/* socket_accept - listen for remote network connections on a given port */

/*
   Copyright (C) 2020 Free Software Foundation, Inc.

   This file is part of GNU Bash.
   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bashtypes.h"
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "typemax.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "loadables.h"

static int accept_bind_variable (char *, int);

int
accept_builtin (list)
     WORD_LIST *list;
{
  SHELL_VAR *v;
  intmax_t isock;
  int opt;
  char *tmoutarg, *fdvar, *rhostvar, *rhost, *bindaddr;
  unsigned short uport;
  int clisock;
  struct sockaddr_in server, client;
  socklen_t clientlen;
  struct timeval timeval;
  struct linger linger = { 0, 0 };

  rhostvar = tmoutarg = fdvar = rhost = bindaddr = (char *)NULL;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "b:r:t:v:")) != -1)
    {
      switch (opt)
	{
	case 'b':
	  bindaddr = list_optarg;
	  break;
	case 'r':
	  rhostvar = list_optarg;
	  break;
	case 't':
	  tmoutarg = list_optarg;
	  break;
	case 'v':
	  fdvar = list_optarg;
	  break;
	CASE_HELPOPT;
	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }
  
  list = loptend;

  /* Validate input and variables */
  if (tmoutarg)
    {
      long ival, uval;
      opt = uconvert (tmoutarg, &ival, &uval, (char **)0);
      if (opt == 0 || ival < 0 || uval < 0)
	{
	  builtin_error ("%s: invalid timeout specification", tmoutarg);
	  return (EXECUTION_FAILURE);
	}
      timeval.tv_sec = ival;
      timeval.tv_usec = uval;
      /* XXX - should we warn if ival == uval == 0 ? */
    }

  if (list == 0)
    {
      builtin_usage ();
      return (EX_USAGE);
    }

  if (legal_number (list->word->word, &isock) == 0 || isock < 0 || isock > TYPE_MAXIMUM (unsigned short))
    {
      builtin_error ("%s: invalid port number", list->word->word);
      return (EXECUTION_FAILURE);
    }

  unbind_variable (fdvar);
  if (fdvar == 0)
    fdvar = "ACCEPT_FD";

  clientlen = sizeof (client);
  if ((clisock = accept (isock, (struct sockaddr *)&client, &clientlen)) < 0)
    {
      builtin_error ("client accept failure: %s", strerror (errno));
      return (EXECUTION_FAILURE);
    }

  accept_bind_variable (fdvar, clisock);
  if (rhostvar)
    {
      rhost = inet_ntoa (client.sin_addr);
      v = builtin_bind_variable (rhostvar, rhost, 0);
      if (v == 0 || readonly_p (v) || noassign_p (v))
	builtin_error ("%s: cannot set variable", rhostvar);
    }

  return (EXECUTION_SUCCESS);
}

static int
accept_bind_variable (varname, intval)
     char *varname;
     int intval;
{
  SHELL_VAR *v;
  char ibuf[INT_STRLEN_BOUND (int) + 1], *p;

  p = fmtulong (intval, 10, ibuf, sizeof (ibuf), 0);
  v = builtin_bind_variable (varname, p, 0);		/* XXX */
  if (v == 0 || readonly_p (v) || noassign_p (v))
    builtin_error ("%s: cannot set variable", varname);
  return (v != 0);
}

char *accept_doc[] = {
	"Accept a network connection on a specified port.",
	""
	"This builtin allows a bash script to act as a TCP/IP server.",
	"",
	"Options, if supplied, have the following meanings:",
	"    -b address    use ADDRESS as the IP address to listen on; the",
	"                  default is INADDR_ANY",
	"    -t timeout    wait TIMEOUT seconds for a connection. TIMEOUT may",
	"                  be a decimal number including a fractional portion",
	"    -v varname    store the numeric file descriptor of the connected",
	"                  socket into VARNAME. The default VARNAME is ACCEPT_FD",
	"    -r rhost      store the IP address of the remote host into the shell",
	"                  variable RHOST, in dotted-decimal notation",
	"",
	"If successful, the shell variable ACCEPT_FD, or the variable named by the",
	"-v option, will be set to the fd of the connected socket, suitable for",
	"use as 'read -u$ACCEPT_FD'. RHOST, if supplied, will hold the IP address",
	"of the remote client. The return status is 0.",
	"",
	"On failure, the return status is 1 and ACCEPT_FD (or VARNAME) and RHOST,",
	"if supplied, will be unset.",
	"",
	"The server socket fd will be closed before accept returns.",
	(char *) NULL
};

struct builtin socket_accept_struct = {
	"socket_accept",		/* builtin name */
	accept_builtin,		/* function implementing the builtin */
	BUILTIN_ENABLED,	/* initial flags for builtin */
	accept_doc,		/* array of long documentation strings. */
	"socket_accept [-b address] [-t timeout] [-v varname] [-r addrvar ] port",		/* usage synopsis; becomes short_doc */
	0			/* reserved for internal use */
};
