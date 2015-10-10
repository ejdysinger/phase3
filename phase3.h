/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200

typedef struct semaphore{
	int maxValue;
	int current;
	mailLine * waitList;
} semaphore;

typedef struct mailLine
{
    int PID;
    // a pointer to the location in memory where the sent message is stored
    char msg[MAX_MESSAGE];
    // the max message size that can be held
    int msgSize;
    // the status of the mailLine object
    int status;
    // a pointer to the next mailLine object
    struct mailLine * next;
}mailLine;

#endif /* _PHASE3_H */

