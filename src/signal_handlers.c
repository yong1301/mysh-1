#include "signal_handlers.h"
#include <signal.h>
#include <stdio.h>

void catch_sigint(int signalNo)
{
  printf("\nYour input Ctrl+C\n");
  printf("Shell doesn't close.\n");
  signal(SIGINT, catch_sigint);
}


void catch_sigtstp(int signalNo)
{
  printf("\nYour input Ctrl+Z, but it doesn't move.\n");
  signal(SIGTSTP, catch_sigtstp);
}
