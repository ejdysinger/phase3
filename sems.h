
#define MAX_SEMS 200



typedef struct semaphore
{
	int maxValue;
	int current;
	int status;
	semWaiter waitList[MAXPROC];
	int seMbox;
} semaphore;

typedef struct semWaiter
{
	int PID;
	semWaiter next;
}semWaiter;
