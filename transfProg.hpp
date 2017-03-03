/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: transfProg.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-03T17:08:21-05:00
* @License: MIT
*/



#ifndef __EFT_TRANSFER__
#define __EFT_TRANSFER__


#include "bankAccount.hpp"
#include "workerQueue.hpp"
#include "debugMacros.hpp"

// Macros
#define           LINE_BUFFER                   50
#define           MAX_WORKERS                   10000

// Thread Data
typedef struct threadData {
  int64_t threadID;                             // Each thread has it's own ID
  workerQueue_t EFTRequests;                // Each thread has it's own queue
  bankAccountPool_t *accountPool;           // Each thread has access to common account pool
} threadData_t;

// Functions for threads processing
int64_t spawnThreads(pthread_t *threads, threadData_t *threadDataPool, \
  bankAccountPool_t *accountPool, int64_t NumberOfThreads);
void askThreadsToExit(threadData_t *threadData, bankAccountPool_t &accountPool,\
   int64_t NumberOfThreads, int64_t lastAssignedID);

#endif
