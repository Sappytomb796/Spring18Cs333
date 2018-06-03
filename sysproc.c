#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start. 
int
sys_uptime(void)
{
  uint xticks;
  
  xticks = ticks;
  return xticks;
}

//Turn of the computer
int
sys_halt(void){
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

#ifdef CS333_P1
int
sys_date(void)
{
  struct rtcdate *d;

  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
    return -1;

  argptr(0, (void*)(&d), sizeof(*d));
  cmostime(d);
  
  return 0;
}
#endif

#ifdef CS333_P2
int
sys_getuid(void)
{
  return proc->uid;
}

int
sys_getgid(void)
{
  return proc->gid;
}

// Make sure if there is no parent to
// return our own pid.
int
sys_getppid(void)
{
  if(proc->parent)
    return proc->parent->pid;
  else
    return proc->pid;
}

int sys_setuid(void)
{
  uint num;

  argint(0, (int*)&num);
  //if it's a valid input, load it and return success
  if(0 <= num && num <= 32767){
    proc->uid = num;
    return 0; }
  return -1;
}

int sys_setgid(void)
{
  uint num;

  argint(0, (int*)&num);
  if(0 <= num && num <= 32767){
    proc->gid = num;
    return 0; }
  return -1;
}

int
sys_getprocs(void)
{
  int temp;
  uint size;
  struct uproc ** table = 0x00;

  //Start by checking the two base cases; no ints in the frame
  //and nothing else waiting in the stack
  if(argint(0, (int *)&size) < 0)
    return -1;

  if(argptr(1, (char **)table, sizeof(struct uproc *)) < 0)
    return -1;

  temp = getproc(size, *table);
  return temp;
}
#endif
#ifdef CS333_P3P4
int
sys_setpriority(void)
{
  int pid  = 0x00;
  int prio = 0x00;

  if(argint(0, (int *)&pid) < 0)  return -1;
  if(argint(1, (int *)&prio) < 0) return -1;

  return set_prio(pid, prio);
}
#endif
