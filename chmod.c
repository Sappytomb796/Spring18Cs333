#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf(1, "Please enter an octal number to set the mode, followed by a valid file\n");
    exit(); }

  //Enforces the use of 0 numbers.  so if it is 777, it will not accept
  if(strlen(argv[1]) < 4){
    printf(1, "Must enter in a valid format of 4 octal bytes\n");
    exit(); }

  //returns -1 if not octal
  int num = atoo(argv[1]);

  //this accounts if it's not octal because clever atoo() usage.
  if(num < 0){
    printf(1, "Enter a valid octal mode to set this to.\n");
    exit(); }
  if(num > 1777){
    printf(1, "Enter a valid octal mode to set this to.\n");
    exit(); }


  if(chmod(argv[2], num) == -1){
    printf(1, "please enter valid input\n");
    exit(); }

  exit();
}

#endif
