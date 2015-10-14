#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>

/* -------------------------- Prototypes ------------------------------------- */


/* -------------------------- Globals ------------------------------------- */

int debugFlag = 1;
/* ------------------------------------------------------------------------ */


int
start2(char *arg)
{
    int pid;
    int status;
    /*
     * Check kernel mode here.
     */

    /*
     * Data structure initialization as needed...
     */


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
    pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

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
    int pid;
    pid = fork1(name, func, arg, stacksize, priority);
    if(pid<0)
        return -1;
    else
        return pid;
}/* spawnReal */

void wait(systemArgs *args){
    if(args->number != SYS_WAIT){
        if (debugFlag){
            USLOSS_Console("wait(): Attempted to wait a process with wrong sys call number: %d.\n", args->number);
        }
        return;
    }
    int pid;
    int status;
    pid = waitReal(&status);
    args->arg1 = (void *)pid;
    args->arg2 = status;
    args->arg4 = pid==-1 ? pid : 0;
}

int waitReal(int * status){
    int pid;
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
    terminateReal();
}

void terminateReal(){
    
}



/* handlers include :
 *
 */
