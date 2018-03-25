#ifdef CS333_P2
#include "types.h"
#include "user.h"

#define DEF_UID 69
#define DEF_GID 420

int main()
{
  uint uid, gid, ppid;
  int pass = 0;
  
  uid = getuid();
  if(uid != DEF_UID)
    pass = -1;
  printf(2, "Current UID is: %d\n", uid);
  printf(2, "Setting UID to 100\n");
  setuid(100);
  uid = getuid();
  printf(2, "New UID is: %d\n", uid);
  if(uid != 100)
    pass = -1;

  gid = getgid();
  if(gid != DEF_GID)
    pass = -1;
  printf(2, "Current GID is: %d\n", gid);
  printf(2, "Setting GID to 50\n");
  setgid(50);
  gid = getgid();
  printf(2, "New GID is: %d\n", gid);
  if(gid != 50)
    pass = -1;

  printf(2, "Now testing for invalid id's:\n");
  uid = getuid();
  printf(2, "Current UID is: %d\n", uid);
  printf(2, "Setting UID to -5\n");
  setuid(-5);
  uid = getuid();
  printf(2, "New UID is: %d\n", uid);
  if(uid != 100)
    pass = -1;

  gid = getgid();
  printf(2, "Current GID is: %d\n", gid);
  printf(2, "Setting GID to -5\n");
  setgid(-5);
  gid = getgid();
  printf(2, "New GID is: %d\n", gid);
  if(gid != 50)
    pass = -1;

  uid = getuid();
  printf(2, "Current UID is: %d\n", uid);
  printf(2, "Setting UID to 32777\n");
  setuid(32777);
  uid = getuid();
  printf(2, "New UID is: %d\n", uid);
  if(uid != 100)
    pass = -1;

  gid = getgid();
  printf(2, "Current GID is: %d\n", gid);
  printf(2, "Setting GID to 32777\n");
  setgid(32777);
  gid = getgid();
  printf(2, "New GID is: %d\n", gid);
  if(gid != 50)
    pass = -1;
  
  ppid = getppid();
  printf(2, "Parent process is: %d\n", ppid);
  if(ppid == -1)
    pass = -1;

  if(pass == -1)
    printf(2, "This test failed somewhere.\n");
  else
    printf(2, "This test PASSES.\n");
  
  exit();
}
#endif
