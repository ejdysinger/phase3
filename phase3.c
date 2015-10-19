#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include "phase3.h"
#include <usyscall.h>
#include "sems.h"

/* -------------------------- Prototypes ------------------------------------- */
void semV(systemArgs *args);
void semFree(systemArgs *args);
void getTimeOfDay(systemArgs *args);
void cpuTime(systemArgs *args);
void getPID(systemArgs *args);
void nullSys3(systemArgs *args);
void spawn(systemArgs *args);
void wait(systemArgs *args);
void terminate(systemArgs *args);

/* -------------------------- Globals ------------------------------------- */
struct ProcStruct ProcTableThree[MAXPROC];

struct semaphore SemTable[MAXSEMS];

void (*systemCallVec[MAXSYSCALLS])(systemArgs *args);

int debugFlag = 1;
/* ------------------------------------------------------------------------ */


int start2(char *arg){
    int pid;
    int status;
    /* Check kernel mode here. */
    if(!(USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()))
            USLOSS_Halt(1);
    /* Data structure initialization as needed... */
    int iter = 0;
    /* the semaphore table */
    for(iter = 0; iter < MAXSEMS;iter++)
    {
    	SemTable[iter].status = INACTIVE;
    }
    /* set all of sys_vec to nullsys3 */
    for(iter = 0;iter < MAXSYSCALLS; iter++)
    {
    	systemCallVec[iter] = nullSys3;
    }
    
    for(iter = 0; iter < MAXPROC ; iter++){
        ProcTableThree[iter].status = INACTIVE;
    }

    /* place appropriate system call handlers in appropriate slots */
    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_WAIT] = wait;
    systemCallVec[SYS_TERMINATE] = terminate;
    systemCallVec[SYS_SEMCREATE] = semCreate;
    systemCallVec[SYS_SEMP];
    systemCallVec[SYS_SEMV] = semV;;
    systemCallVec[SYS_SEMFREE] = semFree;
    systemCallVec[SYS_GETTIMEOFDAY] = getTimeOfDay;
    systemCallVec[SYS_CPUTIME] = cpuTime;
    systemCallVec[SYS_GETPID] = getPID;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    pid = spawnReal("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

} /* start2 */


void spawn(systemArgs *args){
    if(args->number != SYS_SPAWN){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process with wrong sys call number: %d.\n", args->number);
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    // Priority out of bounds
    if(args->arg4>6 || args->arg4<1){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process with priority out of bounds.\n");
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    
    if(args->arg3 < USLOSS_MIN_STACK){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process stack size too small.\n");
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    int result;
    result = spawnReal(args->arg5, args->arg1, args->arg2, args->arg3,args->arg4);
    
    args->arg1 = (void *)result;
    args->arg4 = (void *)0;
    
}
int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, int priority){
    int kidpid;
    kidpid = fork1(name, func, arg, stacksize, priority);
    if(kidpid<0)
        return -1;
    else{
        int i;
        int parentpid = getpid();
        for(i=0;i<MAXPROC;i++){
            if(ProcTableThree[parentpid%MAXPROC].children[i] == INACTIVE){
                ProcTableThree[parentpid%MAXPROC].children[i] = kidpid;
            }
        }
        strcpy(ProcTableThree[kidpid%MAXPROC].name, name);
        ProcTableThree[kidpid%MAXPROC].pid = kidpid;
        ProcTableThree[kidpid%MAXPROC].status = ACTIVE;
        memset(ProcTableThree[kidpid%MAXPROC].children, INACTIVE, MAXPROC*sizeof(ProcTableThree[kidpid%MAXPROC].children[0]));
        return kidpid;
    }
    
}/* spawnReal */

//void wait(systemArgs *args){
//    if(args->number != SYS_WAIT){
//        if (debugFlag){
//            USLOSS_Console("wait(): Attempted to wait a process with wrong sys call number: %d.\n", args->number);
//        }
//        return;
//    }
//    int pid;
//    int status;
//    pid = waitReal(&status);
//    args->arg1 = (void *)pid;
//    args->arg2 = status;
//    args->arg4 = pid==-1 ? pid : 0;
//}

int waitReal(int * status){
    int pid;
    ProcTableThree[getpid()%MAXPROC].status = WAIT_BLOCKED;
    pid = join(status);
    if(pid == -2){
        pid = -1;
    }
    return pid;
}

void terminate(systemArgs *args){
    if(args->number != SYS_TERMINATE){
        if (debugFlag){
            USLOSS_Console("terminate(): Attempted to terminate a process with wrong sys call number: %d.\n", args->number);
        }
        return;
    }
    int currentpid = getpid();
    terminateReal(currentpid);
}

void terminateReal(int pid){
    int i;
    for(i=0;i<MAXPROC;i++){
        if(ProcTableThree[pid%MAXPROC].children[i] == INACTIVE){
            break;
        }
        else{
            terminateReal(ProcTableThree[pid%MAXPROC].children[i]);
            ProcTableThree[pid%MAXPROC].children[i] = INACTIVE;
        }
    }
    zap(pid);
    /* Remove process from ProcessTable after it quits */
    ProcTableThree[pid%MAXPROC].status = INACTIVE;
    
    
    /* If the process last zapped was Start3 then halt b/c all user processes are done. */
    if(strcmp(ProcTableThree[pid%MAXPROC].name, "start3")==0){
        if(debugFlag){
            USLOSS_Console("terminateReal(): Start 3 terminated, halting...\n");
        }
        USLoss_halt(0);
    }
    
}

void semCreate(systemArgs *args){
    if(args->number != SYS_SEMCREATE){
        if (debugFlag){
            USLOSS_Console("semCreate(): Attempted to semCreate wrong sys call number: %d.\n", args->number);
        }
        return;
    }
}




/* handlers include : */
/* ------------------------------------------------------------------------
   Name - SemV
   Purpose - performs a "V" operation on a semaphore
   Parameters - a struct of arguments
   Returns - -1 if semaphore handle is invalid, 0 otherwise
   Side Effects - none
   ----------------------------------------------------------------------- */
void semV(systemArgs *args)
{
	if(args->number != SYS_SEMV)
	{
		if (debugFlag)
			USLOSS_Console("semV(): Attempted a \"V\" operation on a semaphore with wrong sys call number: %d.\n", args->number);
		return;
	}
	/* retrieves the semaphore location from the args struct */
	int semNum;
	semNum = semVHelper((int *)args->arg1);
	/* if the semaphore handle is invalid return -1 */
	args->arg4 =  semNum == -1 ? -1 : 0;
}

/* helper function for semV */
int semVHelper(int * semNum)
{
	struct semaphore * target = SemTable[semNum % MAX_SEMS];
	/* check if valid semaphore */
	if(target->status == INACTIVE)
		return -1;
	/* increment if possible */
	if(target->current < target->maxValue)
		target->current++;
	/* block on the semaphore and add to semaphore waitlist if not */
	else
		semBlockMe(target, getPID);
	MboxSend(target->seMbox, NULL, NULL);
	return 0;
}

/* adds a process to the list of processes blocked on a particular sempahore */
void semBlockMe(struct semaphore * target, int PID)
{
	struct semWaiter * temp = target->waitList;
	if(temp->PID == INACTIVE)
		target->waitList.PID = PID;
	else if(temp->next.PID == INACTIVE)
		temp->next.PID = PID;
	else
	{
		for(;temp->next->PID != INACTIVE)
	}
}

/* ------------------------------------------------------------------------
   Name - SemFree
   Purpose - frees a semaphore; terminates any processes waiting on the
   	   	   	   semaphore
   Parameters - a struct of arguments
   Returns - -1 if sem handle invalid, 1 if processes were blocked on the
   	   	   	   sem, 0 otherwise
   Side Effects - none
   ----------------------------------------------------------------------- */
void semFree(systemArgs *args)
{
	if(args->number != SYS_SEMFREE)
	{
		if (debugFlag)
			USLOSS_Console("semFree(): Attempted a \"Free\" operation on a semaphore with wrong sys call number: %d.\n", args->number);
		return;
	}
	int semNum = semFreeHelper((int *)args->arg1);
	/* place return value into arg4 */
	args->arg4 = semNum;
}

int semFreeHelper(int * semNum)
{
	struct semaphore * target = SemTable[semNum % MAX_SEMS];
	int reply;
	/* check if valid semaphore */
	if(target->status == INACTIVE)
		return -1;
	if(target->waitList.waitList != NULL){
		reply = 1;
		for(;)
		terminateReal(target->waitList.waitList->PID);
	}
	else
		reply = 0;
}

/* ------------------------------------------------------------------------
   Name - GetTimeOfDay
   Purpose - returns the value of the USLOSS time-of-day clock
   Parameters - a struct of arguments
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void getTimeOfDay(systemArgs *args)
{
	/* place time of day in args*[4] */

}

/* ------------------------------------------------------------------------
   Name - CPUTime
   Purpose - returns the cpu time of the current process
   Parameters - a struct of arguments
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void cpuTime(systemArgs *args)
{

}

/* ------------------------------------------------------------------------
   Name - getPID
   Purpose - returns the PID of the current process
   Parameters - a struct of arguments
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void getPID(systemArgs *args)
{

}

/* ------------------------------------------------------------------------
   Name - nullSys3
   Purpose - handles calls to uninstantiated system calls by terminating
   	   	     calling process
   Parameters - a struct of arguments
   Returns - nothing
   Side Effects - terminates the offending process
   ----------------------------------------------------------------------- */
void nullSys3(systemArgs *args)
{
	USLOSS_Console("nullSys3(): Invalid System call: %d;\n Terminating process/n", args->number);
}
