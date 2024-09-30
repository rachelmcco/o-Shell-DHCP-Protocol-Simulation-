#include "builtins.h"
#include "process.h"
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// No command line can be more than 100 characters
#define MAXLENGTH 100

void
shell (FILE *input)
{
  hash_init (100);
  hash_insert ("?", "0");
  char buffer[MAXLENGTH];
  char *cmdArray[50];
  char *token;

  while (1)
    {
      // Print the cursor and get the next command entered
      printf ("$ ");
      memset (buffer, 0, sizeof (buffer));
      if (fgets (buffer, MAXLENGTH, input) == NULL)
        {
          break;
        }
      if (input != stdin)
        {
          printf ("%s", buffer);
        }
      if (strncmp (buffer, "echo", 4) == 0)
        {
          echo (buffer + 5);
        }
      else if (strncmp (buffer, "pwd", 3) == 0)
        {
          pwd ();
        }
      else if (strncmp (buffer, "cd", 2) == 0)
        {
          buffer[strcspn (buffer, "\n")] = '\0';

          if (chdir (buffer + 3) != 0)
            {
              perror ("cd error");
            }
        }
      else if (strncmp (buffer, "which", 5) == 0)
        {
          which (buffer);
        }
      else if (strncmp (buffer, "quit", 4) == 0)
        {
          break;
        }
      else if (strncmp (buffer, "export", 6) == 0)
        {
          export(buffer + 7);
        }
      else if (strncmp (buffer, "unset", 5) == 0)
        {
          unset (buffer + 6);
        }
      else
        {
          token = strtok (buffer, " ");
          int i = 0;
          while (token != NULL && i < 49)
            {
              cmdArray[i] = token;
              token = strtok (NULL, " ");
              i++;
            }
          cmdArray[i] = NULL; // Null-terminate the array
          __pid_t pid = -1;
          waitpid (pid, NULL, 0);
          posix_spawn (&pid, cmdArray[0], NULL, NULL, cmdArray, NULL);
          waitpid (pid, NULL, 0);
        }
    }
  printf ("\n");
  hash_destroy ();
}