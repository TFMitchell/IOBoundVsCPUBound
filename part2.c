/*
* Description: Project 2 Part 2 - Making child processes from an input.txt. Concurrently run them, and send signals to all of them.
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
#include <signal.h>

void sigFunc(int sig, pid_t *pids, int pidsLen);

int main(int argc, char **argv)
{
  char *readBuf, *token;
  char *arguments[10];
  FILE *file;
  size_t len = 1024;
  char *savePtr;
  int i, c;
  pid_t pids[10];

  int sig;
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1); //gonna be waiting for SIGUSR1
  sigprocmask(SIG_BLOCK, &set, NULL);

  if (argc != 3 || strcmp(argv[1], "-f")) //verifying argv
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

  c = 0; //keeps track of child number
  while (getline(&readBuf, &len, file) >= 0) //reading the file
  {
    token = strtok_r(readBuf, " \n", &savePtr);

    for (i = 0; i < 10; i++) //re-initializing args for child process every loop
    {
      arguments[i] = NULL;
    }

    i = 0;
    while (token != NULL) //setting args for child to launch
    {
      arguments[i] = token;
      token = strtok_r(NULL, " \n", &savePtr);
      i++;
    }

    if ((pids[c] = fork()) < 0) //setting value of child in parent
    {
      printf("Error forking. Trying next command.\n");
      continue;
    }

    if (pids[c] == 0) //inside child process
    {
      sigwait(&set, &sig); //wait for SIGUSR1

      if (execvp(arguments[0], arguments) == -1)
      {
        printf("Error starting program. Trying next command.\n");
        free(readBuf);
        fclose(file);
        exit(0);
      }
    }
    c++; //next space in pids array
  }

  sleep(3);

  //sending SIGUSR1
  printf("Waking up all child processes...\n");
  sigFunc(SIGUSR1, pids, c);

  sleep(3);

  //suspending processes
  printf("Suspending all child processes...\n");
  sigFunc(SIGSTOP, pids, c);

  sleep(3);

  //continuing
  printf("Continuing all child processes...\n");
  sigFunc(SIGCONT, pids, c);

  //waiting for children to finish
  for (i = 0; i < c; i++)
  {
    waitpid(pids[i], 0, 0);
  }

  fclose(file);
  free(readBuf);

  exit(0);
}

void sigFunc(int sig, pid_t *pids, int pidsLen)
{
  for (int i = 0; i < pidsLen; i++)
  {
    if (kill(pids[i], sig))
      printf("Error sending %d.\n", sig);
  }
}
