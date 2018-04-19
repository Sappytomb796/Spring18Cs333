#ifdef CS333_P2
#include "types.h"
#include "user.h"

void
printTime(int start, int end){
  int partial = end - start;
  int whole = 0;
  while(partial >= 100){
    ++whole;
    partial -= 100; }

  if(partial < 10)
    printf(1, "%d.0%d", whole, partial);
  else
    printf(1, "%d.%d", whole, partial);
}

int
main(int argc, char *argv[])
{
  int start = uptime();
  int end   = 0;
  int temp  = 1;
  int rc = 0;

  if(argv[1] == 0x00)
    ;
  else
    temp = fork();

  if (temp < 0)
    exit();

  if(temp == 0)
    rc = exec(argv[1] , argv +1);
  else
    wait();
  end = uptime();

  if(rc >= 0){
    if(argv[1] != 0x0)
      printf(1, "%s ", argv[1]);
    printf(1, "ran in ");
    printTime(start, end);
    printf(1, " seconds\n"); }
  else{
    printf(1, "ERROR: %s failed\n", argv[1]);
    exit(); }

  exit();
}
#endif
