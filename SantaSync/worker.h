#pragma once

#define FIRST_MEETING_RED_WORKERS_COUNT 1
#define SECOND_MEETING_GREEN_WORKERS_COUNT 2

// Names of working elves counters
#define SHARED_GREEN_WORKERS_COUNTER_NAME "greenWorkersCounter"
#define SHARED_GREEN_WORKERS_PRE_COUNTER_NAME "greenWorkersPreCounter"
#define SHARED_RED_WORKERS_COUNTER_NAME "redWorkersCounter"
#define SHARED_RED_WORKERS_PRE_COUNTER_NAME "redWorkersPreCounter"

// Names of semaphores for workers
#define GREEN_WORKERS_SEMAPHORE_NAME "/greenWorkersSem"
#define RED_WORKERS_SEMAPHORE_NAME "/redWorkersSem"

// Time worker waits before returning to work
#define WORKER_WAITING_TIME_SECONDS 10

// Types of working elves
typedef enum workerType
{
	RED = 1,
	GREEN =2
} workerType;

// Macro, depending on elf type will give string representation
#define GET_TYPE_STR(X) (X == RED) ? "RED" : "GREEN" 

// Worker routine
//	   type
//     id - number
int workerRoutine(workerType type, int id);

