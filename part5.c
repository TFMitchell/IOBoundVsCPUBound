/*
* Description: Project 2 Part 5 - Attempts to implement dynamic scheduling to make processes from input.txt run faster. Examines ratio of user to system time.
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

#define maxProc 10

void sigFunc(int sig, pid_t pid);
void alarmHandler(int sig);
void sigchildHandler(int sig);
void scheduler();

enum statuses{INVALID, NEW, RUNNING, STOPPED, FINISHED}; //available status to each process

struct PCB { //two pieces of data for each process
  pid_t pid;
  enum statuses status;
  int recommendedQuantum; //in microseconds
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

    if ((tmpPID = fork()) < 0) //setting value of child in parent
    {
      printf("Error forking. Trying next command.\n");
      continue; //without advancing i
    }

    else if (tmpPID == 0) //inside child process
    {
      sigwait(&set, &sig); //wait for SIGUSR1

      if (execvp(arguments[0], arguments) == -1)
      {
        printf("Error starting program.\n");
        free(readBuf);
        fclose(file);
        exit(0);
      }
    }
    else //inside parent
    {
      processes[c].pid = tmpPID;
      processes[c].status = NEW;
      processes[c].recommendedQuantum = 200000;
    }
    c++; //next space in pids array
  }

  sleep(1);

  processes[0].status = RUNNING; //launching first process
  sigFunc(SIGUSR1, processes[0].pid);

  signal(SIGALRM, alarmHandler); //listening for .2 second intervals
  ualarm(200000, 0);

  scheduler();

  system("clear");

  fclose(file);
  free(readBuf);

  exit(0);
}

void sigFunc(int sig, pid_t pid)
{
  if (kill(pid, sig))
    printf("Error sending %d to %d.\n", sig, pid);
}

void alarmHandler(int sig)
{
  if (processes[currentProc].status == RUNNING) //if current process is still running...
  {
    processes[currentProc].status = STOPPED;
    sigFunc(SIGSTOP, processes[currentProc].pid); //stop it
  }
  currentProc++; //go to next process

  while (processes[currentProc].status == INVALID //if it's invalid or finished, keep going
          || processes[currentProc].status == FINISHED)
  {
    if (++currentProc == maxProc) //unless we reach the end of the array, then reset
      currentProc = 0;
  }

  if (processes[currentProc].status == NEW) //for new, it's SIGUSR1
    sigFunc(SIGUSR1, processes[currentProc].pid);
  else
    sigFunc(SIGCONT, processes[currentProc].pid); //for stopped, it's SIGCONT

  processes[currentProc].status = RUNNING; //set to running

  ualarm(processes[currentProc].recommendedQuantum, 0); //give it recommendedQuantum to run
}

void sigchildHandler(int sig) //every time child exits
{
  for (int i = 0; i < maxProc; i++)
  {
    if (waitpid(processes[i].pid, 0, WNOHANG))
      processes[i].status = FINISHED;
  }
}

void scheduler()
{
  FILE *file;
  char *buf = (char*) malloc (1024 * sizeof(char));
  char *tokens[52];
  int utime, stime;
  size_t len = 1024;
  char *savePtr;
  int i, c;
  char allFinishedFlag = 0;

  while (allFinishedFlag == 0)
  {
    sleep(1);
    system("clear");
    printf("\n-----------------------------Process Info---------------------------\n");
    printf("   PID      Name     State     Parent     Utime     Stime     Quantum\n");

    allFinishedFlag = 1;
    for (c = 0; c < maxProc; c++)
    {
      if (processes[c].status == FINISHED
            || processes[c].status == INVALID)
          continue;

      allFinishedFlag = 0;
      sprintf(buf, "/proc/%d/stat", processes[c].pid);

      if ((file = fopen(buf, "r")) == NULL)
        continue;

      if (getline(&buf, &len, file) > 0)
      {
        tokens[0] = strtok_r(buf, " ", &savePtr);
        i = 1;
        while ((tokens[i] = strtok_r(NULL, " ", &savePtr)) != NULL)
        {
          i++;
        }
        printf("  %-5s   %-10s   %-1s        %-5s     %-5s     %-5s     %d\n", tokens[0], tokens[1], tokens[2], tokens[3], tokens[13], tokens[14], processes[c].recommendedQuantum);

        utime = atoi(tokens[13]);
        stime = atoi(tokens[14]);

        if (stime == 0); //can't divide by 0

        else if (utime / stime < 15 && processes[c].recommendedQuantum > 100000) //scheduler logic
          processes[c].recommendedQuantum -= 50000;

        else if (processes[c].recommendedQuantum < 500000)//if (utime / stime > .5)
          processes[c].recommendedQuantum += 50000;
      }
      fclose(file);
    }
  }

  free(buf);
  return;

}
