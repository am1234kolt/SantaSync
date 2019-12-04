#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include "santa.h"
#include "worker.h"
#include "collector.h"
#include "helper.h"

int santaRoutine() {

	sem_t * santa_sem;

	sem_t * counter_mutex;
	sem_t * pre_counter_mutex;
	sem_t * stop_sem;

	sem_t * collectors_sem;
	sem_t * green_workers_sem;
	sem_t * red_workers_sem;

	sem_t * meeting_end_sem;

	// Counters in shared memory for elves, who entered a meeting with santa

	int collectors_shm;
	int * p_collectors_counter;

	int green_workers_shm;
	int * p_green_workers_counter;

	int red_workers_shm;
	int * p_red_workers_counter;

	//Counters in shared memory for elves, waiting for permission to enter

	int pre_collectors_shm;
	int * p_collectors_pre_counter;

	int pre_green_workers_shm;
	int * p_green_workers_pre_counter;

	int pre_red_workers_shm;
	int * p_red_workers_pre_counter;

	int nap_counter = 0;
	

	queue wishList;

	printf("Santa's thread is starting...\n");

	// Create wishlist
	initPresentsQueue(&wishList, CREATE_QUEUE);

	// Create( open) counters of collectors in shared memory

	getSharedCounter(SHARED_COLLECTORS_COUNTER_NAME, &collectors_shm, &p_collectors_counter);
	
	// Create( open) counters of green elves in shared memory
	getSharedCounter(SHARED_GREEN_WORKERS_COUNTER_NAME, &green_workers_shm, &p_green_workers_counter);

	// Create( open) counters of red elves in shared memory
	getSharedCounter(SHARED_RED_WORKERS_COUNTER_NAME, &red_workers_shm, &p_red_workers_counter);
			
	// Create( open) counters of waiting  collectors in shared memory
	getSharedCounter(SHARED_COLLECTORS_PRE_COUNTER_NAME, &pre_collectors_shm, &p_collectors_pre_counter);

	// Create( open) counters of waiting green elves in shared memory
	getSharedCounter(SHARED_GREEN_WORKERS_PRE_COUNTER_NAME, &pre_green_workers_shm, &p_green_workers_pre_counter);

	// Create( open) counters of waiting red elves in shared memory
	getSharedCounter(SHARED_RED_WORKERS_PRE_COUNTER_NAME, &pre_red_workers_shm, &p_red_workers_pre_counter);

	///////////////////////////////////////////////////////////////
	// Mutex for counters
	if ((counter_mutex = sem_open(COUNTERS_MUTEX_NAME, O_CREAT, 0777, 1)) == SEM_FAILED)
	{
		perror("Semaphore for counters has not been created!");
		return 1;
	}
	if ((pre_counter_mutex = sem_open(PRE_COUNTERS_MUTEX_NAME, O_CREAT, 0777, 1)) == SEM_FAILED)
	{
		perror("Semaphore for pre counters has not been created!");
		return 1;
	}
	
	///////////////////////////////////////////////////////////////
	// semaphores for process synchronization
	if ((santa_sem = sem_open(SANTA_SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	{
		perror("Semaphore for Santa has not been created!");
		return 1;
	}

	if ((collectors_sem = sem_open(COLLECTORS_SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	{
		perror("Semaphore for collectors has not been created!");
		return 1;
	}
	
	if ((green_workers_sem = sem_open(GREEN_WORKERS_SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	{
		perror("Semaphore for green workers has not been created!");
		return 1;
	}

	if ((red_workers_sem = sem_open(RED_WORKERS_SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	{
		perror("Semaphore for red workers has not been created!");
		return 1;
	}

	if ((meeting_end_sem = sem_open(MEETING_END_SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	{
		perror("Semaphore for meeting end has not been created!");
		return 1;
	}
	// Semaphore to prevent meeting disruption
	if ((stop_sem = sem_open(ELVES_STOPPING_SEMAPHORE_NAME, O_CREAT, 0777, 1)) == SEM_FAILED)
	{
		perror("Semaphore for stopping elves has not been created!");
		return 1;
	}
		
	// Set counters to zero
	sem_wait(counter_mutex);
	*p_collectors_counter = 0;
	*p_green_workers_counter = 0;
	*p_red_workers_counter = 0;
	sem_post(counter_mutex);

	sem_wait(pre_counter_mutex);
	*p_collectors_pre_counter = 0;
	*p_green_workers_pre_counter = 0;
	*p_red_workers_pre_counter = 0;
	sem_post(pre_counter_mutex);

	printf("Santa has created all sycnronization stuff...\n");

	while (1) {
		
		// If counter is set to zero, meetings are not coming up
		if (nap_counter == 0) {
			printf("Santa is napping..Zzzz...\n");
		}
		// Wait till necessary amount of elves would collect, signal this semaphore
		sem_wait(santa_sem);
		
		sem_wait(counter_mutex); // Protect counters from  simultaneous access
		printf("Santa has woken up. Counters:\n"); // Santa has woken up, check number of eleves
		printf(" Collectors: %d\n", *p_collectors_counter);
		printf(" Green Workers: %d\n", *p_green_workers_counter);
		printf(" Red Workers: %d\n", *p_red_workers_counter);

		// Check which type of meeting to start
		if (
			*p_collectors_counter >= FIRST_MEETING_MINIMUM_COLLECTORS_COUNT 
			&& 
			*p_red_workers_counter >= FIRST_MEETING_RED_WORKERS_COUNT
		)
		{
			printf("Santa has launched FirstMeeting!\n");

			// Notify correct amount of participants
			for (size_t i = 0; i < *p_collectors_counter; i++)
			{
				sem_post(collectors_sem);
			}
			sem_post(red_workers_sem);
			// Meeting
			sleep(FIRST_MEETING_TIME);
			printf("Santa has ended FirstMeeting!\n");

			// Finish meeting
			for (size_t i = 0; i < (*p_collectors_counter) + FIRST_MEETING_RED_WORKERS_COUNT; i++)
			{
				sem_post(meeting_end_sem);
			}

			// Participant leave
			*p_collectors_counter = 0;
			(*p_red_workers_counter)-= FIRST_MEETING_RED_WORKERS_COUNT;
					
		}
		if (*p_green_workers_counter >= SECOND_MEETING_GREEN_WORKERS_COUNT) 
		{
			
			printf("Santa has launched SecondMeeting!\n");
			// Notify correct amount of participants
			for (size_t i = 0; i < *p_green_workers_counter; i++)
			{
				sem_post(green_workers_sem);
			}

			// Meeting
			sleep(SECOND_MEETING_TIME);
			printf("Santa has ended SecondMeeting\n");

			// Finish meeting
			for (size_t i = 0; i < SECOND_MEETING_GREEN_WORKERS_COUNT; i++)
			{
				sem_post(meeting_end_sem);
			}

			// participants leave
			*p_green_workers_counter -= SECOND_MEETING_GREEN_WORKERS_COUNT;
			
		}
		sem_wait(pre_counter_mutex); // protection of counters for waiting elves
		// If there is enough people for the next meeting , when the last one is finished
		//nap counter is increased , so that at the start of the cycle
		//santa will know if he can start another meeting
		if (
			*p_collectors_pre_counter >= FIRST_MEETING_MINIMUM_COLLECTORS_COUNT
			&&
			*p_red_workers_pre_counter >= FIRST_MEETING_RED_WORKERS_COUNT
			||
			*p_green_workers_pre_counter >= SECOND_MEETING_GREEN_WORKERS_COUNT
			)
			nap_counter++;
		// Otherwise fall asleep, no elves
		else {
			nap_counter=0;
		}
		sem_post(stop_sem); // Waiting elves can come in
		sem_post(pre_counter_mutex);
		
		sem_post(counter_mutex);

		// If santa did 4 meeting in a row, does to rest
		if (nap_counter == SANTA_MEETINGS_COUNT_MAX) {
			printf("Santa is sick and tired. He has taken required nap!\n");
			sleep(SANTA_NAPPING_TIME);
			nap_counter = 0;
		}
		
	}
	
	return 0;
}

