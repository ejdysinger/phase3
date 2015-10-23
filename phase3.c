#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include "phase3.h"
#include <usyscall.h>
#include "sems.h"
#include <string.h>
#include <stdio.h>

/* -------------------------- Prototypes ------------------------------------- */
extern int start3(char *arg);
void semV(systemArgs *args);
int semVReal(int semNum);
void semFree(systemArgs *args);
int semFreeReal(int * semNum);
void getTimeOfDay(systemArgs *args);
void cpuTime(systemArgs *args);
void getPID(systemArgs *args);
void nullSys3(systemArgs *args);
void spawn(systemArgs *args);
void myWait(systemArgs *args);
void terminate(systemArgs *args);
void semCreate(systemArgs *args);
void semP(systemArgs *args);
void terminateReal(int initialipid, int currentpid);
int semCreateReal(int value);
int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, int priority);
int waitReal(int * status);
void semPReal(int index);
void toUserMode();
void spawnLaunch(void);

/* -------------------------- Globals ------------------------------------- */
struct ProcStruct ProcTableThree[MAXPROC];

struct semaphore SemTable[MAXSEMS];
int semsUsed;

void (*systemCallVec[MAXSYSCALLS])(systemArgs *args);

int debugFlag = 0;
/* ------------------------------------------------------------------------ */


int start2(char *arg){
    int pid;
    int status;
    /* Check kernel mode here. */
    if(!(USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()))
            USLOSS_Halt(1);
    /* Data structure initialization as needed... */
    semsUsed = 0;
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
        ProcTableThree[iter].procMbox=-1;
        ProcTableThree[iter].parentPid=-1;
        memset(ProcTableThree[iter].children, INACTIVE, sizeof(ProcTableThree[iter].children));
    }

    /* place appropriate system call handlers in appropriate slots */
    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_WAIT] = myWait;
    systemCallVec[SYS_TERMINATE] = terminate;
    systemCallVec[SYS_SEMCREATE] = semCreate;
    systemCallVec[SYS_SEMP] = semP;
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
    if(debugFlag){
        USLOSS_Console("start2(): after spawnReal(). \n");
    }
    
    pid = waitReal(&status);
    //toUserMode();

    return pid;

} /* start2 */
/* USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE) */

/* handlers include : */
void spawn(systemArgs *args){
    if(debugFlag){
        USLOSS_Console("spawn(): Starting spawn. \n");
    }
    if(args->number != SYS_SPAWN){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process with wrong sys call number: %d.\n", args->number);
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    // Priority out of bounds
    if((int)args->arg4>6 || (int)args->arg4<1){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process with priority out of bounds.\n");
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    
    if((int)args->arg3 < USLOSS_MIN_STACK){
        if (debugFlag){
            USLOSS_Console("spawn(): Attempted to spawn process stack size too small.\n");
        }
        args->arg4 = (void *)-1;
        args->arg1 = (void *)-1;
        return;
    }
    int result;
    result = spawnReal(args->arg5, args->arg1, args->arg2, (int)args->arg3,(int)args->arg4);
    
    args->arg1 = result;
    args->arg4 = (void *)0;
    
    toUserMode();
    
    
}
int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, int priority){
    if(debugFlag){
        USLOSS_Console("spawnReal(): Starting spawnReal. \n");
    }
    int kidpid;
    char * msg = NULL;
    kidpid = fork1(name, spawnLaunch, arg, stacksize, priority);
    if(debugFlag){
        USLOSS_Console("spawnReal(): After fork1. \n");
    }
    if(kidpid<0)
        return -1;
    int i;
    int parentpid = getpid();
    for(i=0;i<MAXPROC;i++){
        if(ProcTableThree[parentpid%MAXPROC].children[i] == INACTIVE){
            ProcTableThree[parentpid%MAXPROC].children[i] = kidpid;
            if(debugFlag){
                USLOSS_Console("spawnReal(): Parent: %d, Child: %d\n", parentpid,kidpid);
            }
            break;
        }
    }
    strcpy(ProcTableThree[kidpid%MAXPROC].name, name);
    ProcTableThree[kidpid%MAXPROC].status = ACTIVE;
    ProcTableThree[kidpid % MAXPROC].func = func;
    ProcTableThree[kidpid % MAXPROC].arg = arg;
    ProcTableThree[kidpid % MAXPROC].parentPid = parentpid;
    //strcpy(ProcTableThree[kidpid % MAXPROC].arg, arg);
    ProcTableThree[kidpid % MAXPROC].pid = kidpid;
    memset(ProcTableThree[kidpid%MAXPROC].children, INACTIVE, MAXPROC*sizeof(ProcTableThree[kidpid%MAXPROC].children[0]));
    if(ProcTableThree[kidpid%MAXPROC].procMbox == -1){
        int mboxID;
        mboxID = MboxCreate(0, 50);
        if (mboxID<0){
            if (debugFlag){
                USLOSS_Console(("spawnReal(): Mailbox was unable to be created.\n"));
            }
        }
        ProcTableThree[kidpid%MAXPROC].procMbox = mboxID;
    }
    MboxSend(ProcTableThree[kidpid%MAXPROC].procMbox, msg, 0);
    if (debugFlag){
        USLOSS_Console("spawnReal(): After mboxsend.\n");
    }
    if (debugFlag){
        USLOSS_Console("spawnReal(): kidpid: %d.\n", kidpid);
    }
    return kidpid;
    
    
}/* spawnReal */

/* ------------------------------------------------------------------------
   Name - SpawnLaunch()
   Purpose - spawnLaunch() will begin executing the function passed to Spawn.
    		 spawnLaunch() will need to switch to user-mode before allowing
    		 user code to execute.
   Parameters - a struct of arguments
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void spawnLaunch(){
    
    if(ProcTableThree[getpid()%MAXPROC].procMbox == -1){
        int mboxID;
        mboxID = MboxCreate(0, 50);
        if (mboxID<0){
            if (debugFlag){
                USLOSS_Console(("spawnReal(): Mailbox was unable to be created.\n"));
            }
        }
        ProcTableThree[getpid()%MAXPROC].procMbox = mboxID;
    }
    if(debugFlag){
        USLOSS_Console("spawnLaunch(): Starting. \n");
    }
	int result;
    char * msg = NULL;
    if(debugFlag){
        USLOSS_Console("spawnLaunch(): mboxreceive from box id: %d. \n", ProcTableThree[getpid()%MAXPROC].procMbox);
    }
	MboxReceive(ProcTableThree[getpid()%MAXPROC].procMbox, msg, 0);
    //MboxRelease(ProcTableThree[getpid()%MAXPROC].procMbox);

    if (debugFlag){
        USLOSS_Console("spawnLaunch(): started function: %s\n", ProcTableThree[getpid()%MAXPROC].name);
    }
	/* switch to user mode */
	int launchPID = getpid();
    toUserMode();
	if (debugFlag){
		USLOSS_Console("spawnLaunch(): yO dawg, we here.\n");
	}
	/* Call the function passed to fork1, and capture its return value */
	result = ProcTableThree[launchPID%MAXPROC].func(ProcTableThree[launchPID%MAXPROC].arg);

//    systemArgs sysArg;
//    sysArg.number = SYS_TERMINATE;
//    terminate(&sysArg);
    Terminate(5);
}

void myWait(systemArgs *args){
    if(debugFlag){
        USLOSS_Console("myWait(): Starting.\n");
    }
    if(args->number != SYS_WAIT){
        if (debugFlag){
            USLOSS_Console("wait(): Attempted to wait a process with wrong sys call number: %d.\n", args->number);
        }
        toUserMode();
        return;
    }
    int pid;
    int status;
    pid = waitReal(&status);
    args->arg1 = pid;
    args->arg2 = status;
    args->arg4 = pid==-1 ? &pid : 0;
    toUserMode();
}

int waitReal(int * status){
    if(debugFlag){
        USLOSS_Console("waitReal(): Starting.\n");
    }
    int pid;
    ProcTableThree[getpid()%MAXPROC].status = WAIT_BLOCKED;
    pid = join(status);
    if(pid == -2){
        pid = -1;
    }
    return pid;
}

void terminate(systemArgs *args){
	if (debugFlag){
		USLOSS_Console("terminate(): yO dawg, we here.\n");
	}
	if (debugFlag){
        USLOSS_Console("terminate(): starting\n");
    }
    if(args->number != SYS_TERMINATE){
        if (debugFlag){
            USLOSS_Console("terminate(): Attempted to terminate a process with wrong sys call number: %d.\n", args->number);
        }
        toUserMode();
        return;
    }
    
    int currentpid = getpid();
    terminateReal(currentpid, currentpid);
    //toUserMode();
}

void terminateReal(int initialpid, int pid){
    if (debugFlag){
        USLOSS_Console("terminateReal(): starting for pid: %d\n", pid);
        int j;
        USLOSS_Console("terminateReal(): children of pid: %d\n", pid);
        for (j=0;j<MAXPROC;j++){
            if(ProcTableThree[pid%MAXPROC].children[j] != INACTIVE){
                USLOSS_Console("terminateReal(): %d\n", ProcTableThree[pid%MAXPROC].children[j]);
            }
        }
    }
    int i;
    for(i=0;i<MAXPROC;i++){
        if(ProcTableThree[pid%MAXPROC].children[i] == INACTIVE){
            continue;
        }
        else{
            terminateReal(initialpid, ProcTableThree[pid%MAXPROC].children[i]);
            if (debugFlag){
                USLOSS_Console("terminateReal(): zapping pid: %d\n", ProcTableThree[pid%MAXPROC].children[i]);
            }
            zap(ProcTableThree[pid%MAXPROC].children[i]);
            ProcTableThree[pid%MAXPROC].children[i] = INACTIVE;
        }
    }
    if (debugFlag){
        USLOSS_Console("terminateReal(): outside of for loop for pid: %d\n", pid);
    }
    /* Remove process from ProcessTable after it quits */
    ProcTableThree[pid%MAXPROC].status = INACTIVE;
    
    if(initialpid == pid){
        if (debugFlag){
            USLOSS_Console("terminateReal(): quitting pid: %d\n", initialpid);
        }
        int i;
        int parentPid = ProcTableThree[pid % MAXPROC].parentPid;
        for(i=0;i<MAXPROC;i++){
            if (ProcTableThree[parentPid % MAXPROC].children[i] == pid){
                ProcTableThree[parentPid % MAXPROC].children[i] = INACTIVE;
            }
        }
        quit(initialpid);
    }
    
    /* If the process last zapped was Start3 then halt b/c all user processes are done. */
    if(strcmp(ProcTableThree[pid%MAXPROC].name, "start3")==0){
        if(debugFlag){
            USLOSS_Console("terminateReal(): Start 3 terminated, halting...\n");
        }
        USLOSS_Halt(0);
    }
    
}

void semCreate(systemArgs *args){
    if(args->number != SYS_SEMCREATE){
        if (debugFlag){
            USLOSS_Console("semCreate(): Attempted to semCreate wrong sys call number: %d.\n", args->number);
        }
        toUserMode();
        return;
    }
    
    if(semsUsed>MAXSEMS || args->arg1 < 0){
        args->arg4 = (void *)-1;
        return;
    }
    int index;
    index = semCreateReal((int)args->arg1);
    args->arg1 = index;
    args->arg4 = 0;
    toUserMode();
}

int semCreateReal(int value){
    struct semaphore newSem;
    // create mboxes for P operation mutex and for semaphore manipulation
    int mboxId = MboxCreate(0, 50);
    int mutexBoxId = MboxCreate(0, 50);
    newSem.seMboxID = mboxId;
    newSem.mutexBox = mutexBoxId;
    newSem.status=ACTIVE;
    newSem.head = -1;
    newSem.tail = -1;
    newSem.blockedProc = 0;
    newSem.value = value;
    newSem.maxValue = value;
    int i;
    for(i=0;i<MAXSEMS;i++){
        if(SemTable[i].status == INACTIVE){
            SemTable[i] = newSem;
            break;
        }
    }
    semsUsed++;
    return i;
}

void semP(systemArgs *args){
	if (debugFlag){
		USLOSS_Console("semP(): started.\n");
	}
	if(args->number != SYS_SEMP){
        if (debugFlag){
            USLOSS_Console("semP(): Attempted to semP with wrong sys call number: %d.\n", args->number);
        }
        toUserMode();
        return;
    }
    long index;
    index = ((long)args->arg1);
    if (debugFlag){
		USLOSS_Console("semP(): Retrieved semaphore number: %d.\n", index);
	}
    if(index > MAXSEMS-1 || SemTable[index].status == INACTIVE){
        if(debugFlag){
            USLOSS_Console("semP(): Semaphore inactive or index out of bounds at index: %d.\n", index);
        }
        args->arg4 = (void *)-1;
        return;
    }
    semPReal(index);
    args->arg4=0;
    toUserMode();
}

/* void semPReal(int index){
    int mboxId = SemTable[index%MAXSEMS].seMboxID;
    char * msg;
    // List is empty
    if(SemTable[index%MAXSEMS].head == -1){
        SemTable[index%MAXSEMS].waitList[0] = getpid();
        SemTable[index%MAXSEMS].head = 0;
        SemTable[index%MAXSEMS].tail = 0;
    }
    // List is full
    else if(SemTable[index%MAXSEMS].head == (SemTable[index%MAXSEMS].tail+1)%MAXPROC){
        if(debugFlag){
            USLOSS_Console("semPReal(): index: %d in SemTable has a full wait list.\n", index%MAXSEMS);
        }
    }
    else{
        SemTable[index%MAXSEMS].tail = (SemTable[index%MAXSEMS].tail+1)%MAXPROC;
        SemTable[index%MAXSEMS].waitList[SemTable[index%MAXSEMS].tail] = getpid();
    }

    MboxReceive(mboxId, msg, 50);
    SemTable[index].value--;
} */

void semPReal(int index){
	struct semaphore * target = &SemTable[index % MAX_SEMS];
	char * msg = "hi";
	while(1){
		/* enter mutex */
		target->blockedProc++;
		MboxSend(target->mutexBox, msg, 0);
		/* if the semaphore has been cleared since the process blocked, terminate */
		/* enter the semaphore mbox */
		MboxSend(target->seMboxID, msg, 0);
		/* if the semaphore has been cleared since the process blocked, terminate */
		if(target->status == INACTIVE){
			MboxCondReceive(target->seMboxID, msg, 0);
			MboxCondReceive(target->mutexBox, msg, 0);
			terminateReal(getpid(), getpid());
			break;
		}
		/* if the semaphore value can be decremented, do so */
		if(target->value > 0){
			target->value--;
			break;
		}
		/* otherwise exit the semaphore and mutex mboxes */
		target->blockedProc--;
		MboxCondReceive(target->seMboxID, msg, 0);
		MboxCondReceive(target->mutexBox, msg, 0);
	}
	/* exit both semaphore and mutex mbox */
	target->blockedProc--;
	MboxCondReceive(target->seMboxID, msg, 0);
	MboxCondReceive(target->mutexBox, msg, 0);
}

/* ------------------------------------------------------------------------
   Name - SemV
   Purpose - performs a "V" operation on a semaphore
   Parameters - a struct of arguments
   Returns - -1 if semaphore handle is invalid, 0 otherwise
   Side Effects - none
   ----------------------------------------------------------------------- */
void semV(systemArgs *args){
	if(args->number != SYS_SEMV)
	{
		if (debugFlag)
			USLOSS_Console("semV(): Attempted a \"V\" operation on a semaphore with wrong sys call number: %d.\n", args->number);
			toUserMode();
		return;
	}
	/* retrieves the semaphore location from the args struct */
	int semNum;
	int semTarg = ((int *)args->arg1);
	if (debugFlag)
		USLOSS_Console("semV(): Received semaphore number: %d.\n", semTarg);
	semNum = semVReal(semTarg);
	/* if the semaphore handle is invalid return -1 */
	args->arg4 =  &semNum;
	toUserMode();
}

/* helper function for semV */
int semVReal(int semNum)
{
	if (debugFlag)
			USLOSS_Console("semV(): Started on semapohore number: %d.\n", semNum);
	struct semaphore * target = &SemTable[semNum % MAX_SEMS];
	char * msg = "hi";
	/* enter mutex */
	target->blockedProc++;
	MboxSend(target->mutexBox, msg, 0);
	MboxSend(target->seMboxID, msg, 0);
	/* check if valid semaphore */
	if(target->status == INACTIVE){
		MboxCondReceive(target->seMboxID, msg, 0);
		MboxCondReceive(target->mutexBox, msg, 0);
		return -1;
	}
	/* increment if possible */
	if(target->value < target->maxValue)
		target->value++;
	target->blockedProc--;
	/* exit mutex */
	MboxCondReceive(target->seMboxID, msg, 0);
	MboxCondReceive(target->mutexBox, msg, 0);
	/* return 0 to indicate success */
	return 0;
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
		toUserMode();
		return;
	}
	int semNum = semFreeReal((int *)args->arg1);
	/* place return value into arg4 */
	args->arg4 = &semNum;
	toUserMode();
}

int semFreeReal(int * semNum)
{
	struct semaphore * target = &SemTable[*semNum % MAX_SEMS];
	int reply;
	char * msg = "hi";
	MboxSend(target->mutexBox, msg, 0);
	MboxSend(target->seMboxID, msg, 0);
	/* check if valid semaphore */
	if(target->status == INACTIVE)
		return -1;
	/* set the semaphore status to inactive */
	target->status = INACTIVE;
	/* if there are processes waiting on the mailboxes, free them */
	if(target->blockedProc != 0){
		reply = 1;
		int iter;
		for(iter = target->blockedProc; iter > 0; iter--){
			MboxCondSend(target->mutexBox, "", 0);
			MboxCondSend(target->seMboxID, "", 0);
		}
	}else
		reply = 0;
	MboxCondReceive(target->seMboxID, msg, 0);
	MboxCondReceive(target->mutexBox, msg, 0);
	/* free the mailbox */
	MboxRelease(target->seMboxID);
	MboxRelease(target->mutexBox);
	/* return the status */
	return reply;
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
	args->arg4 = readtime();
	toUserMode();

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
	args->arg1 = readCurStartTime();
	toUserMode();
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
	args->arg1 = getpid();
	toUserMode();
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
	terminateReal(getpid(), getpid());
}

/* adds a process to the list of processes blocked on a particular sempahore
void semAddMe(struct semaphore * target, int PID)
{
	// if head and tail are both -1, the list is empty
	if(target->head == -1 && target->tail == -1){
		target->head = 0;
		target->waitList[target->head] = PID;
	}
	// if tail is -1 and head is not, the list has 1 element
	else if(target->head != -1 && target->tail == -1){
		target->tail = target->head + 1;
		target->waitList[target->tail] = PID;
	}else{
		target->tail = (target->tail + 1) % MAXPROC;
		target->waitList[target->tail] = PID;
	}
}

/* removes the process from the list of processes blocked on a particular
 * semaphore and returns its PID or returns -1
int semRemoveMe(struct semaphore * target)
{
	int reply;
	//if head and tail are both -1, the list is empty
	if(target->head == -1 && target->tail == -1){
		return -1;
	}
	reply = target->waitList[target->head];
	target->waitList[target->head] = 0;
	// if tail is -1 and head is not, the list has 1 element
	if(target->head != -1 && target->tail == -1)
		target->head = -1;
	else
		target->head = (target->head +1) % MAXPROC;

	return reply;
}*/

void toUserMode(){
    
    if(debugFlag){
        USLOSS_Console("toUserMode(): switching to User Mode.\n");
    }
    USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE);
//    unsigned int psr = USLOSS_PsrGet();
//    USLOSS_Console("toUserMode(): PSR before: %d\n", psr & USLOSS_PSR_CURRENT_MODE);
//    psr = (psr & ~1);
//    //psr &= ~(0 << USLOSS_PSR_CURRENT_MODE);
//    USLOSS_PsrSet(psr);
//    USLOSS_Console("toUserMode(): PSR after: %d\n", psr & USLOSS_PSR_CURRENT_MODE);
}
