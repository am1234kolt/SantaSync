#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

#include "santa.h"
#include "worker.h"
#include "collector.h"
#include "helper.h"

// Nameless semaphore to manage access to threads' parameters
sem_t arg_sem;

// Thread function santa
void * santaThread(void * pArg) {
	santaRoutine();
	pthread_exit(NULL);
}

// Thread function collector elves
// Pointer to id is passed in all functions
void * collectorThread(void * pArg) {
	int * pId;
	
	sem_wait(&arg_sem);
	pId = (int*)pArg;
	(*(int *)pArg)++;
	sem_post(&arg_sem);

	collectorRoutine(*pId);
	pthread_exit(NULL);
}

// Green elves' thread
void * greenWorkerThread(void * pArg) {

	int * pId;
	
	sem_wait(&arg_sem);
	pId = (int*)pArg;
	(*(int *)pArg)++;
	sem_post(&arg_sem);

	workerRoutine(GREEN, *pId);
	pthread_exit(NULL);
}

// Red elves' thread
void * redWorkerThread(void * pArg) {

	int * pId;
	
	sem_wait(&arg_sem);
	pId = (int*)pArg;
	(*(int *)pArg)++;
	sem_post(&arg_sem);

	workerRoutine(RED, *pId);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	char c;
	// Variables to store thread numbers
	int * pRedWorkerIdGenerator;
	int * pGreenWorkerIdGenerator;
	int * pCollectorIdGenerator;

	pthread_t tid, tid1, tid2, tid3;
	time_t t;

	srand((unsigned)time(&t));

	pRedWorkerIdGenerator = calloc(sizeof(int), 1);
	pGreenWorkerIdGenerator = calloc(sizeof(int), 1);
	pCollectorIdGenerator = calloc(sizeof(int), 1);

	pthread_create(&tid, NULL, santaThread, NULL);

	// Semaphore init
	sem_init(&arg_sem, 0, 1);
	while (1) {
		scanf("%c", &c);
		if (c == 'q') // Quit
			break;

		if (c == 'g') // Start green worker thread
			pthread_create(&tid1, NULL, greenWorkerThread, pGreenWorkerIdGenerator);

		if (c == 'r') // Start red worker thread
			pthread_create(&tid2, NULL, redWorkerThread, pRedWorkerIdGenerator);

		if (c == 'c') // Start collectors thread
			pthread_create(&tid3, NULL, collectorThread, pCollectorIdGenerator);

		usleep(10);
	}
	printf("Cleaning...\n");

	free(pRedWorkerIdGenerator);
	free(pGreenWorkerIdGenerator);
	free(pCollectorIdGenerator);
	sem_destroy(&arg_sem);

	printf("Quit....\n");

	return 0;
}


