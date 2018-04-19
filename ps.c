#ifdef CS333_P2

#define MAX 3

#include "types.h"
#include "user.h"
#include "uproc.h"

int
main(void)
{
  struct uproc *p = 0x00;
  int rc = 0;
  int i = 0;

  p = (struct uproc *) malloc((MAX * sizeof(struct uproc)));
  rc = getprocs(MAX, &(*p));

  if(rc == 1){
    printf(1, "ERROR\n");
    exit(); }

  printf(1, "\nPID\tUID\tGID\tPPID\tPrio\tElapsed\tTotal\tSTATE\t\tSize\tName\n");
  while(i < rc){
    printf(1, "%d",  p[i].pid);                                   // 1
    printf(1, "\t%d",  p[i].uid);                                 // 2
    printf(1, "\t%d", p[i].gid);                                  // 3
    printf(1, "\t%d",  p[i].ppid);                              // 4
    printf(1, "\t%d", p[i].prio);
    printf(1, "\t%d.%d",
	   p[i].elapsed_ticks/1000, p[i].elapsed_ticks%1000);     // 5
    printf(1, "\t%d.%d",
	   p[i].CPU_total_ticks/1000, p[i].CPU_total_ticks%1000); // 6
    printf(1, "\t%s", p[i].state);                                // 7
    printf(1, "\t%d", p[i].size);                                 // 8
    printf(1, "\t%s", p[i].name);                                 // 9
    i++;
    printf(1, "\n");
  }

  //Lets delete the whole table nao =D
  //for(i = 0; i < rc; i++)
  //  free((void*)p[i]);
  free(p);
  
  exit();
}
#endif
