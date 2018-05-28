// helper file to test requirements

#ifdef CS333_P3P4
#include "types.h"
#include "user.h"

void
forktest(int N)
{
  int n, pid;

  printf(1, "starting fork test\n");

  for(n=0; n < N; n++){
    pid = fork();
    if(pid < 0)
      exit();
    if(pid == 0){
      printf(1, "Parent, breaking\n");
      break; }
  }

  while(1)
    ;
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
