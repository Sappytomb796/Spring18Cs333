
#ifdef CS333_P3P4
#include "types.h"
#include "user.h"

void
forktest(int N)
{
  int n, pid;

  printf(1, "starting fork test to test zombie list\n");


  //Fork and make the child procs, check the pid to
  // see if it's the child or parent.  If it's the
  // child, exit.  If it's the parent first sleep
  // and then wait which will reap the children. This
  // will allow to test the zombie list.

  for(n = 0; n < N; n++){
    pid = fork();

    if(pid < 0)
      printf(1, "error in fork?\n\n");
      
    //if it's the child
    if(pid == 0) exit();
     else if(pid > 0){
      printf(1, "Sleeping, call ctrl-z\n");
      sleep(20 * TPS);
    }
  }

  //The only one that has not exited is the parent,
  // can safely call wait to reap the zombies.
  wait();

  printf(1, "fork test OK\n");
}

int
main(int argc, char **argv)
{
  int N;

  if (argc == 1) {
    printf(2, "Enter number of processes to create\n");
    exit();
  }

  N = atoi(argv[1]);
  forktest(N);
  exit();
}
#endif
