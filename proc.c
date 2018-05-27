#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#ifdef CS333_P3P4
#define MAXBUDGET        100
#define MAXPRIO          5
#define TICKS_TO_PROMOTE 1000           // 10 seconds...?

struct StateLists{
  struct proc * ready[MAXPRIO+1];
  struct proc * readyTail[MAXPRIO+1];
  struct proc * free;
  struct proc * freeTail;
  struct proc * sleep;
  struct proc * sleepTail;
  struct proc * zombie;
  struct proc * zombieTail;
  struct proc * running;
  struct proc * runningTail;
  struct proc * embryo;
  struct proc * embryoTail;
};
#endif
  
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3P4
  struct StateLists pLists;
  uint PromoteAtTime;
#endif
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


#ifdef CS333_P3P4
static void initProcessLists(void);
static void initFreeList(void);
static int stateListAdd(struct proc** head, struct proc** tail, struct proc* p);
static int stateListRemove(struct proc** head, struct proc** tail, struct proc* p);

static int assertState(struct proc * p, enum procstate state);
static int asserPrio(struct proc * p, int i);

// Project 4 prototypes
int promotion();
int search_set(int find_pid, int set_prio, struct proc ** list);
int search_set_ready(int find_pid, int set_prio);
// set_prio is defined in proc.h so it can be accessed from sysproc.c
int set_prio(int find_pid, int set_prio);
#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
#ifdef CS333_P3P4
  p = ptable.pLists.free;
  if(p)
  if(stateListRemove(&ptable.pLists.free, &ptable.pLists.freeTail, p) == 0){
    // Assert the state is correct
    assertState(p, UNUSED);
    goto found;
  }
#else
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
#endif
  release(&ptable.lock);
  return 0;

found:
#ifdef CS333_P3P4
  p->state = EMBRYO; // Change state before adding to the list
  stateListAdd(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p);
#else
  p->state = EMBRYO;
#endif
  
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
  acquire(&ptable.lock);
#ifdef CS333_P3P4
  if(stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p) == 0){
    assertState(p, EMBRYO);
    p->state = UNUSED; // Change state before adding to the list
    stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p); }
  else
    panic("allocproc, kstack");
#else
    p->state = UNUSED;
#endif
  release(&ptable.lock);
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
#ifdef CS333_P1
  p->start_ticks = ticks;
#endif
#ifdef CS333_P2
  //initialize tick counters
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in    = 0;
#endif
#ifdef CS333_P3P4
  p->priority = 0;
  p->budget   = MAXBUDGET; //Start at the max, and we will decrememnt...?
#endif
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

#ifdef CS333_P3P4
  acquire(&ptable.lock);
  initProcessLists();
  initFreeList();

  // Proj 4: Initialize the ptable's promotion timer
  ptable.PromoteAtTime = TICKS_TO_PROMOTE;
  
  release(&ptable.lock);
#endif
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

#ifdef CS333_P2
  p->parent = 0x00;
  p->uid = DEFAULT_UID;
  p->gid = DEFAULT_GID;
#endif

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
  acquire(&ptable.lock);
#ifdef CS333_P3P4
  // Algorithm/ thoughts;
  // This is the first process, we are hand crafting it,
  // so we can put it onto the front of the list manually.  As
  // such we should also set the Tail to point here as well since
  // we are not using the function it won't be done automatically.
  p->next = 0x00;
  assertState(p, EMBRYO);
  if(stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p) == 0){
    p->state = RUNNABLE; // Change state before adding to the list
    stateListAdd(&ptable.pLists.ready[p->priority], &ptable.pLists.readyTail[p->priority], p);
  }
  else
    panic("userinit switch to ready list failure");
#else
  p->state = RUNNABLE;
#endif
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
#ifdef CS333_P3P4
  if(stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, np) == 0){
    assertState(np, EMBRYO);
    np->state = UNUSED;
    if(stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, np) < 0)
      panic("fork; could not add to free");
  }
  else panic("fork; copy process state from p");
#else
    np->state = UNUSED;
#endif
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
#ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
#endif
  
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
#ifdef CS333_P3P4
  if(stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, np) == 0){
    assertState(np, EMBRYO);
    np->state = RUNNABLE; // Change state before adding to the list
    stateListAdd(&ptable.pLists.ready[np->priority], &ptable.pLists.readyTail[np->priority], np);
  }
#else
  np->state = RUNNABLE;
#endif
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
//This is CS333_P3P4 flag turned on.
// Assumes the lock is held!
static void
e_helper(struct proc ** list)
{
  struct proc * temp;
  for(temp = *list; temp; temp = temp->next)
    if(temp->parent == proc)
      temp->parent = initproc;
}

void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);
  // Parent might be sleeping in wait().
  wakeup1(proc->parent);
  // Pass abandoned children to init.
  // We need to search the lists, so will call a
  // helper function and pass the different lists in.
  e_helper(&ptable.pLists.embryo);
  for(int i= 0; i <= MAXPRIO; i++)
    e_helper(&ptable.pLists.ready[i]);
  e_helper(&ptable.pLists.sleep);
  e_helper(&ptable.pLists.running);

  // Zombie list has to be handled differently because of the potential trickery
  // to wake initproc like we see in the original exit loop
  for(p = ptable.pLists.zombie; p; p = p->next)
    if(p->parent == proc){
      p->parent = initproc;
      wakeup1(initproc);
    }
  
  // Jump into the scheduler, never to return.
  if(stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc) == 0){
    assertState(proc, RUNNING);
    proc->state = ZOMBIE;
    stateListAdd(&ptable.pLists.zombie, &ptable.pLists.zombieTail, proc);
  }
  else panic("exit() list failure");

  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
w_helper(struct proc ** list)
{
  struct proc * p;
  int count = 0;
  for(p = *list; p; p = p->next)
    if(p->parent == proc) // If there is a child, 
      count = 1;
  return count;
}

int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;

    for(p = ptable.pLists.zombie; p; p = p->next){
      if(p->parent != proc)
	continue;
      havekids = 1;
      
      // Found one.
      pid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      freevm(p->pgdir);

      //Take it off of the zombie list and move to the free list.
      if(stateListRemove(&ptable.pLists.zombie, &ptable.pLists.zombieTail, p) == 0){
	assertState(p, ZOMBIE);
	p->state = UNUSED;
	stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);    
      }
      else
	panic("wait() list failure");
      
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;

      // Proj 4:  Clear out added fields
      p->priority = 0;
      p->budget   = 0; 
      
      release(&ptable.lock);
      return pid;
    }

    // Now lets check the other lists for kiddos;
    havekids += w_helper(&ptable.pLists.embryo);
    for(int i = 0; i <= MAXPRIO; i++)
      havekids += w_helper(&ptable.pLists.ready[i]);
    havekids += w_helper(&ptable.pLists.running);
    havekids += w_helper(&ptable.pLists.sleep);
    
    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }
    
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void   // Project 3 scheduler.
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();
    idle = 1;  // assume idle unless we schedule a process

    acquire(&ptable.lock);
    for(int i = 0; i <= MAXPRIO; i++)
      for(p = ptable.pLists.ready[i]; p; p = p->next){
	// Check if it's promotion time!!
	if(ticks >= ptable.PromoteAtTime){
	  promotion();
	  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
	}
	
	// Switch to chosen process.  It is the process's job
	// to release ptable.lock and then reacquire it
	// before jumping back to us.
	
	if(stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p) == 0){
	  assertState(p, RUNNABLE);
	  asserPrio(p, i);    // Assert this is on the right priority list
	  p->state = RUNNING; // Change state before adding to the list
	  stateListAdd(&ptable.pLists.running, &ptable.pLists.runningTail, p);
	}
	idle = 0;  // not idle this timeslice
	proc = p;
	switchuvm(p);
	
#ifdef CS333_P2
	p->cpu_ticks_in = ticks;
#endif
	swtch(&cpu->scheduler, proc->context);
	switchkvm();
	
	// Process is done running for now.
	// It should have changed its p->state before coming back.
	proc = 0;
      }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
#ifdef CS333_P2
  proc->cpu_ticks_total += (ticks - proc->cpu_ticks_in);
#endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
#ifdef CS333_P3P4
  if(stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc) == 0){
    assertState(proc, RUNNING);
    proc->state = RUNNABLE; // Change state before adding to the list

    // Proj 4: Adjust the budget, and then check the budget
    //         to see if we should adjust the priority
    proc->budget -= (ticks - proc->cpu_ticks_in);
    if(proc->budget <= 0)
      if(proc->priority < MAXPRIO){
	proc->priority += 1;
	proc->budget    = MAXBUDGET; }
    
    stateListAdd(&ptable.pLists.ready[proc->priority], &ptable.pLists.readyTail[proc->priority], proc);
  }
#else
  proc->state = RUNNABLE;
#endif
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
#ifdef CS333_P3P4
  if(stateListRemove(&ptable.pLists.running, &ptable.pLists.runningTail, proc) == 0){
    assertState(proc, RUNNING);
    proc->state = SLEEPING; // Change state before adding to the list

    // Proj 4: Adjust the budget, and adjust the priority if necessary
    proc->budget -= ticks - proc->cpu_ticks_in;
    if(proc->budget <= 0)
      if(proc->priority < MAXPRIO){
	proc->priority += 1;
	proc->budget    = MAXBUDGET; }
    
    stateListAdd(&ptable.pLists.sleep, &ptable.pLists.sleepTail, proc);
  }
  else
    panic("sleep()");
#else
  proc->state = SLEEPING;
#endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

//PAGEBREAK!
#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.pLists.sleep; p; p = p->next)
    if(p->chan == chan){
      if(stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p) == 0){
	assertState(p, SLEEPING);
	p->state = RUNNABLE; // Change state before adding to the list
	stateListAdd(&ptable.pLists.ready[p->priority], &ptable.pLists.readyTail[p->priority], p);
      }
      else
	panic("wakeup1()");
    }
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
// main logic for searching a single list to set
// kill and return 0 for true if found, and
// return -1 if none through the whole list.
int
k_helper(struct proc ** list, int pid)
{
  struct proc * p;
  for(p = *list; p; p = p->next)
    if(p->pid == pid){
      p->killed = 1;
      return 0;
    }
  return -1;
}

// Created helper to make kill logic more readable
int
kill(int pid)
{
  acquire(&ptable.lock);

  // Search through each list and if found, make
  // sure to release the lock before leaving;
  // Embryo
  if(k_helper(&ptable.pLists.embryo, pid) == 0){
    release(&ptable.lock);
    return 0; }  
  // Ready
  for(int i = 0; i <= MAXPRIO; i++)
    if(k_helper(&ptable.pLists.ready[i], pid) == 0){
      release(&ptable.lock);
      return 0; }
  // Running
  if(k_helper(&ptable.pLists.running, pid) == 0){
    release(&ptable.lock);
    return 0; }
  // Sleeping
  struct proc * p;
  for(p = ptable.pLists.sleep; p; p = p->next)
    if(p->pid == pid){
      stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleepTail, p);
      assertState(p, SLEEPING);
      p->state = RUNNABLE; // Change state before adding to the list
      stateListAdd(&ptable.pLists.ready[p->priority], &ptable.pLists.readyTail[p->priority], p);
      break;
    }

  //None, release lock and return error
  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
#ifdef CS333_P2
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  uint temp;
  
  cprintf("\tPID \tName \tUID \tGID \tPPID \tElapsed \tCPU \tState \tSize \tPCs\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    cprintf("\t%d\t%s\t%d\t%d", p->pid, p->name, p->uid, p->gid);
    if(p->parent == 0x00)
      cprintf("\t%d\t", p->uid);
    else
      cprintf("\t%d\t", p->parent->uid);
    temp = ticks;
    cprintf("%d.%d\t", ((temp - p->start_ticks)/1000), ((temp - p->start_ticks)%1000));
    cprintf(" \t%d", p->cpu_ticks_total);
    cprintf("\t%s\t%d\t", state, p->sz);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
	cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
  cprintf("$ "); //This kills me not having the input that I can type on the command line.
}

#elif CS333_P1
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  uint temp;
  
  cprintf("\tPID \tState \tName \tElapsed \tPCs\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("\t%d \t%s \t%s\t", p->pid, state, p->name);
    temp = ticks;
    cprintf("%d.%d\t", ((temp - p->start_ticks)/1000), ((temp - p->start_ticks)%1000));
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf("\t%p", pc[i]);
    }
    cprintf("\n");
  }
}

#else //Flags off or default
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
#endif

#ifdef CS333_P3P4
static int
stateListAdd(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0) {
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    (*tail)->next = p;
    *tail = (*tail)->next;
    (*tail)->next = 0;
  }

  return 0;
}

static int
stateListRemove(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0 || *tail == 0 || p == 0) {
    return -1;
  }

  struct proc* current = *head;
  struct proc* previous = 0;

  if (current == p) {
    *head = (*head)->next;
    return 0;
  }

  while(current) {
    if (current == p) {
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if (current == 0) {
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if (current == *tail) {
    *tail = previous;
    (*tail)->next = 0;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = 0;

  return 0;
}

static void
initProcessLists(void) {
  for(int i = 0; i <= MAXPRIO; i++){
    ptable.pLists.ready[i] = 0;
    ptable.pLists.readyTail[i] = 0; }
  ptable.pLists.free = 0;
  ptable.pLists.freeTail = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.sleepTail = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.zombieTail = 0;
  ptable.pLists.running = 0;
  ptable.pLists.runningTail = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.embryoTail = 0;
}

static void
initFreeList(void) {
  if (!holding(&ptable.lock)) {
    panic("acquire the ptable lock before calling initFreeList\n");
  }

  struct proc* p;

  for (p = ptable.proc; p < ptable.proc + NPROC; ++p) {
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);
  }
}

// Proj 4: promotion of all ready lists' priority, and all procs
//         that will return to the ready list (sleeping and running)
int
promotion()
{
  if(MAXPRIO == 0)
    return 1; // If there is only one queue we can actually return now without adjustment.

  // Need to know if we should unlock at the end.  This is why I was bugging out was
  // unlocking regardless the first run.
  int holding_lock = holding(&ptable.lock);  
  if(!holding_lock)
    acquire(&ptable.lock);

  struct proc * temp;
  
  for(temp = ptable.pLists.ready[1]; temp; temp = ptable.pLists.ready[1]){
    stateListRemove(&ptable.pLists.ready[1], &ptable.pLists.readyTail[1], temp);
    temp->priority = 0;
    stateListAdd(&ptable.pLists.ready[temp->priority], &ptable.pLists.readyTail[temp->priority], temp);
  }

  for(int i = 2; i <= MAXPRIO; ++i){
    for(temp = ptable.pLists.ready[i]; temp; temp = temp->next)
      temp->priority = (i-1);

    // priority is changed, update the list pointers
    ptable.pLists.ready[i-1]     = ptable.pLists.ready[i];
    ptable.pLists.readyTail[i-1] = ptable.pLists.readyTail[i];

    // Remember to clear out the empty lists to be null pointers as well.
    ptable.pLists.ready[i] = 0x00;
    ptable.pLists.readyTail[i] = 0x00;
  }
  
  if(!holding_lock)
    release(&ptable.lock);

  return 0;  //return code that adjustment was made successfully
}

// The ruberic gives us that it should be an int, but shouldn't it
// really be a uint...?  Potentially this could not reach certain
// numbers as a uint has the extra bit to an int and if a proc has
// a higher numbered pid it wouldn't be able to account for it.  While
// unlikely, it is a corner case.
// Returns 0 for successful completion no matches, 1 for match found and set
int
set_prio(int find_pid, int set_prio)
{
  // Assert the input is all valid before proceding.
  if(find_pid < 0 || set_prio < 0 || set_prio > MAXPRIO)
    return -1;

  int found = 0;
  int holding_lock = holding(&ptable.lock);  

  if(!holding_lock)
    acquire(&ptable.lock);

  if(search_set(find_pid, set_prio, &ptable.pLists.running) == 0)
    found = 1;
  else if(search_set(find_pid, set_prio, &ptable.pLists.sleep) == 0)
    found = 1;
  else if(search_set_ready(find_pid, set_prio) == 0)
    found = 1;
  
  if(!holding_lock)
    release(&ptable.lock);
  return found;
}

// Assumes and enforces holding lock
int
search_set(int find_pid, int set_prio, struct proc ** list)
{
  if(!holding(&ptable.lock)) return -1;
  
  for(struct proc * p = *list; p; p = p->next)
    if(p->pid == find_pid){
      p->priority = set_prio;
      return 0; }
  return -1;
}

// Assumes holding lock
// Note to self; To maintain the invariant, it is important to take
// it off of the list, and then change the priority, and THEN add it
// to the new list!
int
search_set_ready(int find_pid, int set_prio)
{
  if(!holding(&ptable.lock)) return -1;

  for(int i=0; i <= MAXPRIO; i++)
    for(struct proc * p = ptable.pLists.ready[i]; p; p = p->next)
      if(p->pid == find_pid){
	stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);
	p->priority = set_prio;
	stateListAdd(&ptable.pLists.ready[set_prio], &ptable.pLists.readyTail[set_prio], p);
	return 0;
      }
  
  return -1; // Not found
}
#endif

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

#ifdef CS333_P2
char *
convert_state(int n)
{
  switch(n){
  case 0:
    return "Unused  ";
  case 1:
    return "Embryo  ";
  case 2:
    return "Sleeping";
  case 3:
    return "Runnable";
  case 4:
    return "Running ";
  case 5:
    return "Zombie  ";
  }

  return "ERROR";
}


int
getproc(int max, struct uproc * table)
{
  int i = 0;
  int count = 0;
  char * temp;

  acquire(&ptable.lock); //Acquire the lock so nothing is being changed while reading
  while((i < NPROC) && (ptable.proc[i].state != UNUSED) && i < max){
    //read and copy all useful information
    temp = 0x00;
    (table[count]).pid = ptable.proc[i].pid;
    (table[count]).uid = ptable.proc[i].uid;
    (table[count]).gid = ptable.proc[i].gid;
    if(ptable.proc[i].parent->uid == 0x00)
      (table[count]).ppid = ptable.proc[i].uid;
    else
      (table[count]).ppid = ptable.proc[i].parent->uid;

    (table[count]).CPU_total_ticks = ptable.proc[i].cpu_ticks_total;
    (table[count]).elapsed_ticks = (ticks - ptable.proc[i].start_ticks);
    (table[count]).size = ptable.proc[i].sz;

    safestrcpy((table[count]).name, ptable.proc[i].name, sizeof(ptable.proc[i].name));
    temp = convert_state(ptable.proc[i].state);
    safestrcpy((table[count]).state, temp, (strlen(temp) +1));

    ++count;
    ++i;
  }
  release(&ptable.lock); //Make sure to unlock the ptable before leaving!!
  return count;
}
#endif


#ifdef CS333_P3P4
static int
assertState(struct proc * p, enum procstate comp)
{
  if(p->state == comp) //Yay!!!! easy good stuffs!!
    return 1;

  panic("Oh bother...  Wrong state!!");
}

static int
asserPrio(struct proc * p, int i)
{
  if(p->priority == i)
    return 1;

  panic("process was on the wrong priority list!");
}

void
procdumpR(void)
{
  struct proc * temp;

  cprintf("Ready list procs (PID, Budget):\n");
  acquire(&ptable.lock);

  for(int i = 0; i <= MAXPRIO; i++){
    temp = ptable.pLists.ready[i];
    cprintf("Queue: %d\n", i);
    if(!temp)
      cprintf("No processes on the ready[%d] list\n", i);
    for(; temp; temp = temp->next){
      cprintf("(%d, %d) -> ", temp->pid, temp->budget);      
    } 
    cprintf("\n");
  }
  release(&ptable.lock);
  cprintf("$ "); //This kills me not having the input that I can type on the command line.
}

void
procdumpF(void)
{
  struct proc * temp = ptable.pLists.free;
  int count = 0;

  cprintf("Free list size: ");
  acquire(&ptable.lock);
  while(temp){
    ++count;
    temp = temp->next; }
  release(&ptable.lock);
  cprintf("%d procs", count);
  cprintf("\n$ "); //This kills me not having the input that I can type on the command line.
}

void
procdumpS(void)
{
  struct proc * temp = ptable.pLists.sleep;

  cprintf("Sleep list procs:\n");
  acquire(&ptable.lock);
  if(!temp)
    cprintf("No procs on list right now\n");
  else{
    for( ; temp->next; temp = temp->next)
      cprintf("%d -> ", temp->pid);
    cprintf("%d\n", temp->pid); }
  release(&ptable.lock);
  cprintf("$ "); //This kills me not having the input that I can type on the command line.
}

void
procdumpZ(void)
{
  struct proc * temp = ptable.pLists.zombie;

  cprintf("Zombie list procs (PID, PPID):");
  acquire(&ptable.lock);
  if(!temp)
    cprintf("No procs on list right now\n");
  else{
    for( ; temp->next; temp = temp->next)
      cprintf("(%d, %d)-> ", temp->pid, temp->parent->pid);
    cprintf("(%d, %d)\n", temp->pid, temp->parent->pid); }
  release(&ptable.lock);
  cprintf("$ "); //This kills me not having the input that I can type on the command line.
}
#endif
