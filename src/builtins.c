#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *builtins[] = { "echo", "export", "pwd", "unset", "which" };
#define MAX_PATH_LENGTH 1024

// Given a message as input, print it to the screen followed by a
// newline ('\n'). If the message contains the two-byte escape sequence
// "\\n", print a newline '\n' instead. No other escape sequence is
// allowed. If the sequence contains a '$', it must be an environment
// variable or the return code variable ("$?"). Environment variable
// names must be wrapped in curly braces (e.g., ${PATH}).
//
// Returns 0 for success, 1 for errors (invalid escape sequence or no
// curly braces around environment variables).
int
echo (char *message)
{
  int i = 0;
  if (message[0] == '$')
    {
      // Add your desired return code here
      printf ("help");
    }
  while (message[i] != '\0')
    {
      if (message[i] == '\\' && message[i + 1] == 'n')
        {
          printf ("\n");
          i += 2;
        }
      else if (message[i] == '$')
        {
          if (message[i + 1] == '{')
            {
              i += 2; // Move past '$' and '{'
              int j = i;
              while (message[j] != '}')
                {
                  if (message[j] == '\0')
                    {
                      return 1;
                    }
                  j++;
                }
              // Extract the environment variable name
              char envVar[j - i + 1]; // Adjust size
              for (int k = 0; i < j; k++, i++)
                {
                  envVar[k] = message[i];
                }
              envVar[j - i] = '\0'; // Null terminate

              // Retrieve the value of the environment variable
              char *envValue = getenv (envVar);
              if (envValue != NULL)
                {
                  printf ("%s", envValue);
                }
            }
          else
            {
              printf ("$");
              i++;
            }
        }
      else if (message[i] != ' '
               || (message[i] == ' ' && message[i + 1] != ' '))
        {
          printf ("%c", message[i]);
          i++;
        }
      else
        {
          i++;
        }
    }
  return 0;
}
// Given a key-value pair string (e.g., "alpha=beta"), insert the mapping
// into the global hash table (hash_insert ("alpha", "beta")).
//
// Returns 0 on success, 1 for an invalid pair string (kvpair is NULL or
// there is no '=' in the string).
int export(char *kvpair)
{
  kvpair[strlen (kvpair) - 1] = '\0';
  if (kvpair == NULL || !strchr (kvpair, '='))
    {
      return 1;
    }
  char *kvcopy = strdup (kvpair);
  char *token = strtok (kvcopy, "=");
  char *key = token;
  token = strtok (NULL, "=");
  char *val = token;
  hash_insert (key, val);
  free (kvcopy);
  return 0;
}
// Prints the current working directory (see getcwd()). Returns 0.
int
pwd (void)
{
  char path[1024];

  if (getcwd (path, sizeof (path)) != NULL)
    {
      printf ("%s\n", path);
      return 0;
    }
  else
    {
      perror ("getcwd() error");
      return 1;
    }
}
// Removes a key-value pair from the global hash table.
// Returns 0 on success, 1 if the key does not exist.
int
unset (char *key)
{
  if (key == NULL || !hash_find (key))
    {
      return 1;
    }
  hash_remove (key);
  return 0;
}

// Given a string of commands, find their location(s) in the $PATH global
// variable. If the string begins with "-a", print all locations, not just
// the first one.
//
// Returns 0 if at least one location is found, 1 if no commands were
// passed or no locations found.
int
which (char *cmdline)
{
  size_t len = strlen (cmdline);
  while (len > 0 && (cmdline[len - 1] == ' ' || cmdline[len - 1] == '\n'))
    {
      cmdline[len - 1] = '\0';
      len--;
    }

  char strings[][20] = { "cd", "echo", "pwd", "export", "which" };

  int builtin = 0;
  for (int i = 0; i < sizeof (strings) / sizeof (strings[0]); i++)
    {
      if (strcmp (&cmdline[6], strings[i]) == 0)
        {
          printf ("%s: dukesh built-in command\n", &cmdline[6]);
          builtin = 1;
          break;
        }
    }
  if (builtin == 0)
    {
      FILE *pipe = popen (cmdline, "r");

      if (!pipe)
        {
          perror ("popen");
          return 1;
        }

      char buffer[200];
      if (fgets (buffer, sizeof (buffer), pipe) == NULL)
        {
          perror ("fgets");
          pclose (pipe);
          return 1;
        }
      printf ("%s", buffer);
      pclose (pipe);
    }

  return 0;
}
