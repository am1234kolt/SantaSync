#pragma once

// Name of counters for collecting elves
//   Elves counter, entered and waiting for meeting
#define SHARED_COLLECTORS_COUNTER_NAME "collectorsCounter"
//  Elves counter, who did not enter, but are waiting

#define SHARED_COLLECTORS_PRE_COUNTER_NAME "collectorsPreCounter" 

// Min number of collectors for a meeting
#define FIRST_MEETING_MINIMUM_COLLECTORS_COUNT 3

// Name of the semaphore controlling collector
#define COLLECTORS_SEMAPHORE_NAME "/collectorsSem"

// Collecting elf routine
//     int id - number
int collectorRoutine(int id);