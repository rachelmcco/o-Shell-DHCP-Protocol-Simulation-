#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "shell.h"

static bool get_args (int, char **, FILE **);
static void usage (void);

/* This work complies with the JMU Honor Code.
 * References and Acknowledgments: I received no outside
 * help with this programming assignment. */

int
main (int argc, char *argv[])
{
  FILE *script = NULL;
  if (!get_args (argc, argv, &script))
    {
      usage ();
    }
  else if (script == NULL)
    {
      shell (stdin);
    }
  else
    {
      shell (script);
    }
  return EXIT_SUCCESS;
}

/* Parse the command-line arguments. Sets the client/server variables to
   point to a file name (typically in the data/ directory). Can also set
   the bot variable if a second file is used to interact with the
   client/server. If -d was passed, turn on debugging mode to print
   information about state transitions. */
static bool
get_args (int argc, char **argv, FILE **script)
{
  int ch = 0;
  while ((ch = getopt (argc, argv, "b:h")) != -1)
    {
      switch (ch)
        {
        case 'b':
          *script = fopen (optarg, "r");
          break;
        default:
          return false;
        }
    }
  return true;
}

static void
usage (void)
{
  printf ("dukesh, a simple command shell\n");
  printf ("usage: dukesh [-b FILE]\n");
  printf ("  -b FILE    use FILE as a shell script to execute\n");
  printf ("If no script is passed, then the shell should be interactive,\n");
  printf ("processing one command at a time from STDIN.\n");
}