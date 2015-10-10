/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200


#define TRUE 1
#define FALSE 0


#define MAXHANDLERS 1

#define INACTIVE -1

typedef struct mailSlot mailSlot;
typedef struct mailSlot * slotPtr;
typedef struct mailbox   mailbox;
typedef struct mboxProc *mboxProcPtr;


struct mailbox {
    int       mboxID;
    // other items as needed...
    int maxSlots;
    int usedSlots;
    int slotSize;
    slotPtr head;
    slotPtr tail;
    /* a list of processes waiting for slots */
    mailLine * waitList;
};

struct mailSlot {
    int       mboxID;
    int       status;
    // other items as needed...
    int msg_size;
    char message [MAX_MESSAGE];
    slotPtr nextSlot;
};


#endif /* _PHASE3_H */

