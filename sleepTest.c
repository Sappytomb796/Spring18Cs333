// helper file to test requirements for ps command

#ifdef CS333_P3P4
#include "types.h"
#include "user.h"

void
forktest()
{
  int pid;

  printf(1, "starting sleep test\n");

  pid = fork();

  if(pid > 0)
    sleep(100*TPS);

  printf(1, "Done sleeping\n");
  if(pid > 0)
    while(1) ; //infinite loop
  
  if(pid == 0)
    wait();
  if(pid > 0) exit();
}

int
main(int argc, char **argv)
{
  forktest();
  printf(1, "Returned to main, exiting\n");
  exit();
}
#endif
