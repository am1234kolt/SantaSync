#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "helper.h"

int initPresentsQueue(queue * queue, int flag) {
	
	if ((queue->handler_p_shm = shm_open(SHARED_MEMORY_QUEUE_POINTER_NAME, O_CREAT | O_RDWR, 0777)) == -1) {

		return -1;
	}

	if (ftruncate(queue->handler_p_shm, BUFFER_SIZE) == -1) {

		return -1;
	}

	queue->p = (char *)mmap(
		0, BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, queue->handler_p_shm, 0
	);

	if (queue->p == MAP_FAILED) {

		return -1;
	}

	// ƒлина общего списка
	if ((queue->handler_len_shm = shm_open(SHARED_MEMORY_QUEUE_LENGTH_NAME, O_CREAT | O_RDWR, 0777)) == -1) {

		return -1;
	}

	if (ftruncate(queue->handler_len_shm, sizeof(size_t)) == -1) {

		return -1;
	}

	queue->pLen = (size_t *)mmap(
		0, sizeof(size_t), PROT_WRITE | PROT_READ, MAP_SHARED, queue->handler_len_shm, 0
	);

	if (queue->pLen == MAP_FAILED) {

		return -1;
	}

	if ((queue->sem = sem_open(QUEUE_SEMAPHORE_NAME, O_CREAT, 0777, 1)) == SEM_FAILED)
		return -1;

	if (flag == CREATE_QUEUE) {
		*queue->pLen = 0;
		*(queue->p) = '\0';
	}

	return 0;
}

int enqueuePresent(queue * queue, char * name) {

	sem_wait(queue->sem);
	
	if (BUFFER_SIZE - *queue->pLen < strlen(name) + 1) {
		sem_post(queue->sem);
		return -1;
	}
	
	strcpy(queue->p + *queue->pLen, name);
	*queue->pLen += strlen(name) + 1;
	
	sem_post(queue->sem);

	return 0;
}
char * dequeuePresent(queue * queue) {
	size_t len = 0;
	char * res = NULL;
	sem_wait(queue->sem);

	if (*queue->pLen == 0) {
		sem_post(queue->sem);
		return res;
	}
	
	len = strlen(queue->p) + 1;
	res = calloc(len, sizeof(char));
	strcpy(res, queue->p);
	memmove(queue->p, (queue->p)+len, *queue->pLen - len);
	*queue->pLen -= len;
	
	sem_post(queue->sem);

	return res;
}

int getSharedCounter(char * name, int * handler_shm, int** p_counter) {

	
	if ((*handler_shm = shm_open(name, O_CREAT | O_RDWR, 0777)) == -1) {
		
		return -1;
	}

	if (ftruncate(*handler_shm, sizeof(int)) == -1) {
		
		return -1;
	}

	*p_counter = (int *)mmap(
		0, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, *handler_shm, 0
	);

	if (p_counter == MAP_FAILED) {
		
		return -1;
	}

	return 0;
}