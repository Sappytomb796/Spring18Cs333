#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  if(argc != 3) {
    printf(1, "Please enter a number to set the uid, followed by a valid file\n");
    exit(); }

  float num = atoi(argv[1]);

  if(num < 0 || num > 32767){
    printf(1, "please enter a valid number from 0 - 32767\n");
    exit(); }
  
  if (chown(argv[2], num) == -1){
    printf(1, "please enter valid input\n");
    exit(); }

  exit();
}

#endif
