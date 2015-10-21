
#define MAX_SEMS 200

typedef struct semaphore
{
	int maxValue;
	int value;
	int status;
	int waitList[MAXPROC];
	int head;
	int tail;
	int blockedProc;
	int mutexBox; // address of the mutex mbox
	int seMboxID; // address of the sem mbox
} semaphore;
