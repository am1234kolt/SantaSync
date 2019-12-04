#pragma once

// Semaphore names
#define COUNTERS_MUTEX_NAME "/countersMutex"
#define PRE_COUNTERS_MUTEX_NAME "/preCountersMutex"

#define MEETING_END_SEMAPHORE_NAME "/meetingEndSem"
#define SANTA_SEMAPHORE_NAME "/santaSem"

#define ELVES_STOPPING_SEMAPHORE_NAME "/stopSem"
/////////////////////////////////////////////

// Santa's necessary napping time
#define SANTA_NAPPING_TIME 5
// Max number if meetings without rest
#define SANTA_MEETINGS_COUNT_MAX 4

// Length of the meetings
#define FIRST_MEETING_TIME 5
#define SECOND_MEETING_TIME 5

// Действия Санты
int santaRoutine();