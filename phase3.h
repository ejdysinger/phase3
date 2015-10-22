/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200

#define INACTIVE		0
#define ACTIVE			1
#define WAIT_BLOCKED    2

#define TRUE 1
#define FALSE 0


#define MAXHANDLERS 1


struct ProcStruct {
    int pid;
    int parentPid;
    int status;
    int children[MAXPROC];
    char name[MAXNAME];
    int procMbox;
    int (*func)(char *);
    char *arg;
//    procPtr         nextProcPtr;
//    procPtr         childProcPtr;
//    procPtr         nextSiblingPtr;
//    char            name[MAXNAME];     /* process's name */
//    char            startArg[MAXARG];  /* args passed to process */
//    USLOSS_Context  state;             /* current context for process */
//    short           pid;               /* process id */
//    int             priority;
//    int (* start_func) (char *);       /* function where process begins -- launch */
//    char           *stack;
//    unsigned int    stackSize;
//    int             status;            /* READY, BLOCKED, QUIT, etc. */
//    /* other fields as needed... */
//    procPtr        parentPtr;         /* pointer to child's parent; will always be NULL for sentinel */
//    quitNote    *  childStatus;       /* integer value returned by the last child to quit */
//    int             zapStatus[MAXPROC];       /* integer value indicating whether a process is zapped or not */
//    int            runTime;
};
#endif /* _PHASE3_H */

