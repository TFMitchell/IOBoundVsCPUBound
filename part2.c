/*
* Description: Project 2 Part 2
*
* Author: Thomas Mitchell
*
* Date: 11-27-2020
*
* Notes:
* 1. I discussed concepts with Lindsay
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void sigFunc(int sig);

int main(int argc, char **argv)
{
  char *readBuf, *token;
  char *arguments[4];
  FILE *file;
  size_t len = 1024;
  char *savePtr;
  int i, c;
  pid_t pids[10];


  int sig;
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);


  if (argc != 3 || strcmp(argv[1], "-f"))
  {
    printf("Incorrect syntax.\n");
    return 1;
  }
  //else
  if ((file = fopen(argv[2], "r")) == NULL)
  {
    printf("Couldn't open file.\n");
    return 2;
  }
  //else

  for (i = 0; i < 4; i++)
  {
    arguments[i] = NULL;
  }

  readBuf = (char*) malloc(len * sizeof(char));

  c = 0;
  while (getline(&readBuf, &len, file) >= 0)
  {
    token = strtok_r(readBuf, " ", &savePtr);
    i = 0;
    while (token != NULL)
    {
      arguments[i] = token;
      token = strtok_r(NULL, " ", &savePtr);
      i++;
    }

    if ((pids[c] = fork()) < 0)
    {
      printf("Error forking. Trying next command.\n");
      continue;
    }

    if (pids[c] == 0) //inside child process
    {
      sigwait(&set, &sig);

      if (execvp(arguments[0], arguments) == -1)
      {
        printf("Error starting program. Trying next command.\n");
        exit(0);
        continue;
      }
    }
    c++; //next space in pids array
  }


  //sending SIGUSR1
  for (i = 0; i < c; i++)
  {
    if (kill(pids[i], SIGUSR1))
      printf("Error sending SIGUSR1.\n");
  }

  //suspending processes
  for (i = 0; i < c; i++)
  {
    if (kill(pids[i], SIGSTOP))
      printf("Error sending SIGSTOP.\n");
  }

  sleep(3);

  //continuing
  for (i = 0; i < c; i++)
  {
    if (kill(pids[i], SIGCONT))
      printf("Error sending SIGCONT.\n");
  }

  for (i = 0; i < c; i++)
  {
    waitpid(pids[i], 0, 0);
  }

  fclose(file);
  free(readBuf);

  exit(0);
}


void sigFunc(int sig)
{

}