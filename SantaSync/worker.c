#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include "collector.h"
#include "worker.h"
#include "santa.h"
#include "helper.h"

int workerRoutine(workerType type, int id) {
	// Semaphores
	sem_t * santa_sem;

	sem_t * counter_mutex;
	sem_t * pre_counter_mutex;
	sem_t * stop_sem;
	

	sem_t * green_workers_sem;
	sem_t * red_workers_sem;

	sem_t * meeting_end_sem;

	// Counters of shared memory for elves who are having a meeting with santa
	int collectors_shm;
	int * p_collectors_counter;

	int green_workers_shm;
	int * p_green_workers_counter;

	int red_workers_shm;
	int * p_red_workers_counter;

	// Counter of shared memory for elves who are waiting to have a meeting with santa
	int pre_collectors_shm;
	int * p_collectors_pre_counter;

	int pre_green_workers_shm;
	int * p_green_workers_pre_counter;

	int pre_red_workers_shm;
	int * p_red_workers_pre_counter;

	sem_t * needed_sem;
	int * p_needed_counter;
	int * p_needed_pre_counter;

	struct timespec time; // time struct
	queue wishList;
	
	char * present;

	//Prepare nice looking output with tab and ID of elves
	char printf_line_begin[15];
	if (type == RED) {
		sprintf(printf_line_begin, "\t\t\t\t(%d)", id);
	}
	else
		sprintf(printf_line_begin, "\t\t\t\t[%d]", id);
	
	printf("%s Starting %s worker thread...\n", printf_line_begin, GET_TYPE_STR(type));

	// Open exciting queque of presents
	initPresentsQueue(&wishList, OPEN_QUEUE);

	// Create (open) collector counters in shared memory
	getSharedCounter(SHARED_COLLECTORS_COUNTER_NAME, &collectors_shm, &p_collectors_counter);

	// Create (open) green workers counters in shared memory
	getSharedCounter(SHARED_GREEN_WORKERS_COUNTER_NAME, &green_workers_shm, &p_green_workers_counter);

	// Create (open) red workers counters in shared memory
	getSharedCounter(SHARED_RED_WORKERS_COUNTER_NAME, &red_workers_shm, &p_red_workers_counter);

	//Creation( open) counter of collecting elves waiting in shared memory
	getSharedCounter(SHARED_COLLECTORS_PRE_COUNTER_NAME, &pre_collectors_shm, &p_collectors_pre_counter);

	// Creation( open) counter of green workers waiting in shared memory
	getSharedCounter(SHARED_GREEN_WORKERS_PRE_COUNTER_NAME, &pre_green_workers_shm, &p_green_workers_pre_counter);

	// Creation( open) counter of red workers waiting in shared memory
	getSharedCounter(SHARED_RED_WORKERS_PRE_COUNTER_NAME, &pre_red_workers_shm, &p_red_workers_pre_counter);
	
	// Mutex protection counters
	if ((counter_mutex = sem_open(COUNTERS_MUTEX_NAME, 0)) == SEM_FAILED) {
		perror("Counter mutex has not been opened!");
		return 1;
	}
	if ((pre_counter_mutex = sem_open(PRE_COUNTERS_MUTEX_NAME, 0)) == SEM_FAILED) {
		perror("Counter mutex has not been opened!");
		return 1;
	}

	// Semaphores for processes synchronization
	if ((santa_sem = sem_open(SANTA_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Santa semaphore has not been opened!");
		return 1;
	}
	if ((green_workers_sem = sem_open(GREEN_WORKERS_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Workers semaphore has not been opened!");
		return 1;
	}
	if ((red_workers_sem = sem_open(RED_WORKERS_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Workers semaphore has not been opened!");
		return 1;
	}
	if ((meeting_end_sem = sem_open(MEETING_END_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Meeting end semaphore has not been opened!");
		return 1;
	}
	// Prevent meeting destruction(access) while it is happening
	if ((stop_sem = sem_open(ELVES_STOPPING_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Stop semaphore has not been opened!");
		return 1;
	}
	
	// Elf is working  - making presents (removing them from wishlist)
	present = dequeuePresent(&wishList);
	if (present != NULL) {
		printf("%s %s has been handled from wish list\n", printf_line_begin, present);
		free(present);
	}
	else
		printf("%s Wish list is empty - nothing to do...\n", printf_line_begin);
	
	// Depending on worker type choose correct semaphore and counters
	if (type == GREEN) {
		p_needed_counter = p_green_workers_counter;
		needed_sem = green_workers_sem;
		p_needed_pre_counter = p_green_workers_pre_counter;
	}
	else if (type == RED) {
		p_needed_counter = p_red_workers_counter;
		needed_sem = red_workers_sem;
		p_needed_pre_counter = p_red_workers_pre_counter;
	}
	else {
		return -1;
	}
	
	// Wait if meeting can be started, if not go back to work
	clock_gettime(CLOCK_REALTIME, &time);
	time.tv_sec += WORKER_WAITING_TIME_SECONDS;
	
	sem_wait(pre_counter_mutex);
	(*p_needed_pre_counter)++;
	sem_post(pre_counter_mutex);

	printf("%s Checking if worker can enter....\n", printf_line_begin);
	if (sem_timedwait(stop_sem, &time)) { // only wait the amount of time preset
		// takes to long - return back to work
		sem_wait(pre_counter_mutex);
		(*p_needed_pre_counter)--;
		sem_post(pre_counter_mutex);

		printf("%s Meeting is in progress => resume working....\n", printf_line_begin);

		// Close objects of shared memory and semaphores
		munmap(p_collectors_counter, sizeof(int));
		munmap(p_green_workers_counter, sizeof(int));
		munmap(p_red_workers_counter, sizeof(int));
		munmap(p_collectors_pre_counter, sizeof(int));
		munmap(p_green_workers_pre_counter, sizeof(int));
		munmap(p_red_workers_pre_counter, sizeof(int));

		close(collectors_shm);
		close(green_workers_shm);
		close(red_workers_shm);
		close(pre_collectors_shm);
		close(pre_green_workers_shm);
		close(pre_red_workers_shm);

		sem_close(santa_sem);
		sem_close(counter_mutex);
		sem_close(pre_counter_mutex);
		sem_close(stop_sem);
		sem_close(green_workers_sem);
		sem_close(red_workers_sem);
		sem_close(meeting_end_sem);
		////////////////////////////////////////

		return 0;
	}
	// Santa finished a meeting or it did not happen
	printf("%s There is no meeting...\n", printf_line_begin);

	// decrease counter of waiting elves
	sem_wait(pre_counter_mutex);
	(*p_needed_pre_counter)--;
	sem_post(pre_counter_mutex);

	sem_wait(counter_mutex);
	// Increase counter for ones waiting for the meeting
	(*p_needed_counter)++;
	// Print out what number of type entered the room
	printf("%s I'm %d %s worker\n", printf_line_begin, *p_needed_counter, GET_TYPE_STR(type));
	// Check if santa can be woken up
	if (
		*p_collectors_counter >= FIRST_MEETING_MINIMUM_COLLECTORS_COUNT
		&&
		*p_red_workers_counter == FIRST_MEETING_RED_WORKERS_COUNT
		||
		*p_green_workers_counter == SECOND_MEETING_GREEN_WORKERS_COUNT
		) 
	{
		// wake up santa
		printf("%s Santa, wake up!\n", printf_line_begin);
		sem_post(santa_sem);
	}
	else {
		// signal, that other elves can come in
		sem_post(stop_sem);
	}
	sem_post(counter_mutex);
	
	// Can the meeting be start if no, go to work
	clock_gettime(CLOCK_REALTIME, &time);
	time.tv_sec += WORKER_WAITING_TIME_SECONDS;

	printf("%s Checking if meeting will be running soon..\n", printf_line_begin);
	if (sem_timedwait(needed_sem, &time)) {
		sem_wait(counter_mutex);
		(*p_needed_counter)--;
		sem_post(counter_mutex);
		printf("%s Too many time to wait => resume working...\n", printf_line_begin);

		// Close objects of shared memory and semaphores
		munmap(p_collectors_counter, sizeof(int));
		munmap(p_green_workers_counter, sizeof(int));
		munmap(p_red_workers_counter, sizeof(int));
		munmap(p_collectors_pre_counter, sizeof(int));
		munmap(p_green_workers_pre_counter, sizeof(int));
		munmap(p_red_workers_pre_counter, sizeof(int));

		close(collectors_shm);
		close(green_workers_shm);
		close(red_workers_shm);
		close(pre_collectors_shm);
		close(pre_green_workers_shm);
		close(pre_red_workers_shm);

		sem_close(santa_sem);
		sem_close(counter_mutex);
		sem_close(pre_counter_mutex);
		sem_close(stop_sem);
		sem_close(green_workers_sem);
		sem_close(red_workers_sem);
		sem_close(meeting_end_sem);
		////////////////////////////////////////

		return 0;
	}

	printf("%s Participant of Meeting %d!\n", printf_line_begin, type);
	// Waiting till the end of the meeting
	sem_wait(meeting_end_sem);

	printf("%s Resume work.......\n", printf_line_begin);

	// Close object of shared memory and semaphores
	munmap(p_collectors_counter, sizeof(int));
	munmap(p_green_workers_counter, sizeof(int));
	munmap(p_red_workers_counter, sizeof(int));
	munmap(p_collectors_pre_counter, sizeof(int));
	munmap(p_green_workers_pre_counter, sizeof(int));
	munmap(p_red_workers_pre_counter, sizeof(int));

	close(collectors_shm);
	close(green_workers_shm);
	close(red_workers_shm);
	close(pre_collectors_shm);
	close(pre_green_workers_shm);
	close(pre_red_workers_shm);

	sem_close(santa_sem);
	sem_close(counter_mutex);
	sem_close(pre_counter_mutex);
	sem_close(stop_sem);
	sem_close(green_workers_sem);
	sem_close(red_workers_sem);
	sem_close(meeting_end_sem);
	////////////////////////////////////////

	return 0;	
}