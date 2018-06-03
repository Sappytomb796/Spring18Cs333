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
  if(strlen(argv[1]) != 4){
    printf(1, "Must enter in a valid format of 4 octal bytes\n");
    exit(); }

  // Lets do some tricky ricky checking for the octal sizing before turning it into an int with atoo
  //char *mode = argv[1];
  if(!(argv[1][0] == '0' || argv[1][0] == '1')) { // the setuid bit is ONLY allowed to be 0 or 1
    printf(1, "Please enter mode in the range 0000 - 1777\n");
    exit(); }
  for(int i = 1; i < 3; i++){
    if(argv[1][i] < '0' || argv[1][i] > '7'){ // Make sure it's octal
      printf(1, "Please enter mode in the range 0000 - 1777\n");
      exit(); }
  }

  //returns -1 if not octal
  int num = atoo(argv[1]);

  if(num < 0){
    printf(1, "Enter a valid octal mode to set this to.\n");
    exit(); }
  if(num > 1777){
    printf(1, "Enter a valid octal mode to set this to.\n");
    exit(); }

  // Should only return a failure if the file doesn't exist
  if(chmod(argv[2], num) == -1)
    printf(1, "please enter valid input\n");

  exit();
}

#endif
