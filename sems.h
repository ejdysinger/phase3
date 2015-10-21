
#define MAX_SEMS 200

typedef struct semaphore
{
	int maxValue;
	int value;
	int status;
	int waitList[MAXPROC];
	int head;
	int tail;
	int seMboxID; // position in table of the mbox
} semaphore;
