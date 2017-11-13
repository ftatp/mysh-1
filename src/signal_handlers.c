#include <sys/types.h>
#include <stdio.h>
#include "signal_handlers.h"

void catch_sigint(int signalNo)
{
  // TODO: File this!
	//printf("Killing all child\n");
	//kill(0, signalNo);
	//fflush(stdout);
	//printf("\n");

}

void catch_sigtstp(int signalNo)
{
  // TODO: File this!
}
