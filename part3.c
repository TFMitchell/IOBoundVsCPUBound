/*
* Description: Project 2 Part 3
*
* Author: Thomas Mitchell
*
* Date: 11-12-2020
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

#define maxProc 10

void sigFunc(int sig, pid_t pid);
void alarmHandler(int sig);
void sigchildHandler(int sig);

enum statuses{NEW, RUNNING, STOPPED, FINISHED}; //available status to each process

struct PCB { //two pieces of data for each process
  pid_t pid;
  enum statuses status;
};
struct PCB processes[maxProc]; //array of processes
int currentProc = 0; //the one that is currently in execution

int main(int argc, char **argv)
{
  char *readBuf, *token;
  char *arguments[10];
  FILE *file;
  size_t len = 1024;
  char *savePtr;
  int i, c, tmpPID;

  int sig;
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1); //gonna be waiting for SIGUSR1
  sigprocmask(SIG_BLOCK, &set, NULL);
  signal(SIGCHLD, sigchildHandler); //setting up handler to move processes to FINISHED

  for (i = 0; i < maxProc; i++)
  {
    processes[i].pid = 0;
  }

  if (argc != 3 || strcmp(argv[1], "-f")) //verifying argv
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

    if ((tmpPID = fork()) < 0) //setting value of child in parent
    {
      printf("Error forking. Trying next command.\n");
      continue;
    }

    else if (tmpPID == 0) //inside child process
    {
      sigwait(&set, &sig); //wait for SIGUSR1

      if (execvp(arguments[0], arguments) == -1)
      {
        printf("Error starting program.\n");
        exit(0);
      }
    }
    else //inside parent
      processes[c].pid = tmpPID;
      processes[c].status = NEW;
    c++; //next space in pids array
  }

  sleep(3);

  sigFunc(SIGUSR1, processes[0].pid);
  processes[0].status = RUNNING;

  //alarmHandler(0); //starting the ball rolling

  signal(SIGALRM, alarmHandler);
  alarm(1);

  //waiting for children to finish
  for (i = 0; i < c; i++)
  {
    waitpid(processes[i].pid, 0, 0);
  }
  fclose(file);
  free(readBuf);

  exit(0);
}

void sigFunc(int sig, pid_t pid)
{
  if (kill(pid, sig))
    printf("Error sending %d to %d.\n", sig, pid);
  else
    printf("Sent %d to %d.\n", sig, pid);
}

void alarmHandler(int sig)
{
  if (processes[currentProc].status != FINISHED) //if it finished while executing
    sigFunc(SIGSTOP, processes[currentProc].pid);


    while (1)
    {
      if (processes[++currentProc].pid == 0)
      {
        currentProc = 0;
        continue;
      }
      else if (processes[currentProc].status == FINISHED)
      {
        currentProc++;
        continue;
      }
      else if (processes[currentProc].status == NEW)
      {
        processes[currentProc].status = RUNNING;
        sigFunc(SIGUSR1, processes[currentProc].pid);
        break;
      }
      else //if status is stopped
      {
        processes[currentProc].status = RUNNING;
        sigFunc(SIGCONT, processes[currentProc].pid);
        break;
      }
    }

  alarm(1);
}

void sigchildHandler(int sig) //every time child exits
{
  for (int i = 0; i < maxProc; i++)
  {
    if (waitpid(processes[i].pid, 0, WNOHANG))
      processes[i].status = FINISHED;
  }
}
