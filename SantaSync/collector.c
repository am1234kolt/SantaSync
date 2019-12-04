#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "collector.h"
#include "worker.h"
#include "santa.h"
#include "helper.h"

//Generate present with name "Present" + number to a 1000

char * GeneratePresent() {

	char buffer[20];
	char r_str[4];
	char * res;
	int number = 0;
	
	number = rand() % 1000;
	strcpy(buffer, "Present");
	sprintf(r_str, "%d", number);
	strcat(buffer, r_str);
	
	res = calloc(strlen(buffer) + 1,sizeof(char));
	strcpy(res, buffer);

	return res;
}

int collectorRoutine(int id) {
	// Semaphores
	sem_t * santa_sem;
	sem_t * counter_mutex;
	sem_t * pre_counter_mutex;
	sem_t * collectors_sem;
	sem_t * stop_sem;

	sem_t * meeting_end_sem;

	// Counters of shared memory for elves, who are having a meeting with Santa
	int collectors_shm;
	int * p_collectors_counter;

	int red_workers_shm;
	int * p_red_workers_counter;

	// Counters of shared memory for elves who are waiting to be invited for meeting
	int pre_collectors_shm;
	int * p_collectors_pre_counter;

	int pre_red_workers_shm;
	int * p_red_workers_pre_counter;


	queue wishList;

	char * present;

	// Tab + ID
	char printf_line_begin[15];
	sprintf(printf_line_begin, "\t\t{%d}", id);

	printf("%s Starting collector thread...\n", printf_line_begin, id);

	// Open wishlist
	initPresentsQueue(&wishList, OPEN_QUEUE);

	// Creating (opening) collector's counter in shared memory
	getSharedCounter(SHARED_COLLECTORS_COUNTER_NAME, &collectors_shm, &p_collectors_counter);

	//Creating (opening) red elves's counter in shared memory
	getSharedCounter(SHARED_RED_WORKERS_COUNTER_NAME, &red_workers_shm, &p_red_workers_counter);

	// Creating (opening) waiting collector's counter in shared memory
	getSharedCounter(SHARED_COLLECTORS_PRE_COUNTER_NAME, &pre_collectors_shm, &p_collectors_pre_counter);

	//Creating (opening) waiting red elves's counter in shared memory
	getSharedCounter(SHARED_RED_WORKERS_PRE_COUNTER_NAME, &pre_red_workers_shm, &p_red_workers_pre_counter);

	///////////////////////////////////////////////////////////////
	// Mutex to protect counters
	if ((counter_mutex = sem_open(COUNTERS_MUTEX_NAME, 0)) == SEM_FAILED) {
		perror("Counter mutex has not been opened!");
		return 1;
	}
	if ((pre_counter_mutex = sem_open(PRE_COUNTERS_MUTEX_NAME, 0)) == SEM_FAILED) {
		perror("Pre Counter mutex has not been opened!");
		return 1;
	}
	// Semaphores for process synchronization
	if ((santa_sem = sem_open(SANTA_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Santa semaphore has not been opened!");
		return 1;
	}
	if ((collectors_sem = sem_open(COLLECTORS_SEMAPHORE_NAME, 0)) == SEM_FAILED) {
		perror("Collector's semaphore has not been opened!");
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

	// Simulation of adding presents to wishlist
	present = GeneratePresent();
	if (enqueuePresent(&wishList, present) != QUEUE_IS_FULL) { // добавляем подарок в виш лист
		printf("%s %s has been added to wish list\n", printf_line_begin, present);
		free(present);
	}
	else
		printf("%s Whish list is full - nowhere to put whishes...", printf_line_begin);

	// increase counter of waiting elves
	sem_wait(pre_counter_mutex);
	(*p_collectors_pre_counter)++;
	sem_post(pre_counter_mutex);

	// waiting for signal to come in
	sem_wait(stop_sem);

	// entered? decrease counter of waiting
	sem_wait(pre_counter_mutex);
	(*p_collectors_pre_counter)--;
	sem_post(pre_counter_mutex);

	printf("%s There is no meeting yet...\n", printf_line_begin);

	sem_wait(counter_mutex);
	(*p_collectors_counter)++; // Increase counter of ones waiting
	// Print out the number of collectors in the "room"
	printf("%s I`m %d collector\n", printf_line_begin, *p_collectors_counter);
	// If we are ready to start - signal to santa
	if (
		*p_collectors_counter == FIRST_MEETING_MINIMUM_COLLECTORS_COUNT
		&&
		*p_red_workers_counter >= FIRST_MEETING_RED_WORKERS_COUNT
		) 
	{
		printf("%s Santa, wake up!\n", printf_line_begin);
		sem_post(santa_sem);
	}
	else {
		// can not - tell others to enter
		sem_post(stop_sem);
	}
	sem_post(counter_mutex);

	// waiting for the meeting
	sem_wait(collectors_sem);
	printf("%s Participant of Meeting 1!\n", printf_line_begin);
	// waiting for a meeting to end
	sem_wait(meeting_end_sem);
	printf("%s Resume collect...\n", printf_line_begin);

	// Close objects of shared memory and semaphores
	munmap(p_collectors_counter, sizeof(int));
	munmap(p_red_workers_counter, sizeof(int));
	munmap(p_collectors_pre_counter, sizeof(int));
	munmap(p_red_workers_pre_counter, sizeof(int));

	close(collectors_shm);
	close(red_workers_shm);
	close(pre_collectors_shm);
	close(pre_red_workers_shm);

	sem_close(santa_sem);
	sem_close(counter_mutex);
	sem_close(pre_counter_mutex);
	sem_close(stop_sem);
	sem_close(meeting_end_sem);
	////////////////////////////////////////

	return 0;
}