/*
 * Three process test of GetPID.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int Child1(char *);

int semaphore;

int start3(char *arg)
{
   int pid, status;

   printf("start3(): started\n");

   printf("start3(): calling Spawn for Child1a\n");
   Spawn("Child1a", Child1, "Child1a", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1b\n");
   Spawn("Child1b", Child1, "Child1b", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1c\n");
   Spawn("Child1c", Child1, "Child1c", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child2\n");
   Wait(&pid, &status);
   Wait(&pid, &status);
   Wait(&pid, &status);
   printf("start3(): Parent done. Calling Terminate.\n");
   Terminate(8);

   return 0;
} /* start3 */


int Child1(char *my_name) 
{
   int pid;

   printf("%s(): starting\n", my_name);
   GetPID(&pid);
   printf("%s(): pid = %d\n", my_name, pid);
   printf("%s(): done\n", my_name);

   return 9;
} /* Child1 */
