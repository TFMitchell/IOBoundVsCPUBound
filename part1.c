/*
* Description: Project 2 Part 1 - Making processes from an input.txt and parent waits to close them.
*
* Author: Thomas Mitchell
*
* Date: 11-12-2020
*
* Notes:
* 1. N/A
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  char *readBuf, *token;
  char *arguments[10];
  FILE *file;
  size_t len = 1024;
  char *savePtr;
  int i, c;
  pid_t pids[10];

  if (argc != 3 || strcmp(argv[1], "-f")) //checking to make sure argv is proper
  {
    printf("Incorrect syntax.\n");
    return 1;
  }

  else if ((file = fopen(argv[2], "r")) == NULL)
  {
    printf("Couldn't open file.\n");
    return 2;
  }

  readBuf = (char*) malloc(len * sizeof(char));

  c = 0; //c is the process we're keeping track of
  while (getline(&readBuf, &len, file) >= 0)
  {
    token = strtok_r(readBuf, " \n", &savePtr);

    for (i = 0; i < 10; i++) //clearning the arguments for the process to be launched
    {
      arguments[i] = NULL;
    }

    i = 0;
    while (token != NULL) //filling out the arguments
    {
      arguments[i] = token;
      token = strtok_r(NULL, " \n", &savePtr);
      i++;
    }

    if ((pids[c] = fork()) < 0) //setting pids array with child ID
    {
      printf("Error forking. Trying next command.\n");
      continue;
    }

    if (pids[c] == 0) //inside child process
    {
      if (execvp(arguments[0], arguments) == -1)
      {
        printf("Error starting program. Trying next command.\n");
        free(readBuf);
        fclose(file);
        exit(0); //no longer need child
      }
    }
    c++; //next space in pids array for next iteration
  }

  for (i = 0; i < c; i++) //wait for children to finish
  {
    waitpid(pids[i], 0, 0);
  }

  fclose(file);
  free(readBuf);

  exit(0);
}
