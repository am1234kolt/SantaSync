#pragma once
#include <semaphore.h>

/*

 	This file and helper.c contains help structures and functions
 	*/

/*
 	Definitions necessary to describe thread safe queue
    Wishlist is made through this queue

*/
// Buffer of shared memory size for queue
#define BUFFER_SIZE 1024

// Codes for second parameter of initPresentQueue()
// Code of queue creation
#define CREATE_QUEUE 1
// Code of open queue
#define OPEN_QUEUE 2

//Queue is full
#define QUEUE_IS_FULL -1

// Name of object in shared memory (1st- start of the queue)
#define SHARED_MEMORY_QUEUE_POINTER_NAME "queuePointerShm"
// (2nd - length of the queue )
#define SHARED_MEMORY_QUEUE_LENGTH_NAME "queueLenShm"

// Name of Queue access semaphore
#define QUEUE_SEMAPHORE_NAME "queueSem"

// Thread safe queue
typedef struct queue {

	char * p;				// Pointer to the start of the queue
	size_t * pLen;			// Pointer to the length variable
	int handler_p_shm;		// Descriptors
	int handler_len_shm;	//		of shared memory( saved for future deletion)
	sem_t * sem;			// Queue access semaphore

} queue;

// Queue initialization
//		q - pointer to the empty space(filled in by this function)
//		mode - queue open mode
int initPresentsQueue(queue * q, int mode);
// Add on to the queue
//		q - pointer to queue struct
//		name - строка
// Return: -1, if is filled, 0, if adding was successful
int enqueuePresent(queue * q, char * name);
// Pull out string
//		q - pointer to queue struct
// Return: NULL, if is empty, string, if successful
char * dequeuePresent(queue * q);

// Int counter of shared memory
//		name - name of object in shared memory
//		handlet_shm (out) - pointer to object descriptor
//		p_counter (out) - pointer to pointer counter (just return pointer through another pointer)
// Return: -1 - everything is bad, 0 all good)
int getSharedCounter(char * name, int * handler_shm, int** p_counter);