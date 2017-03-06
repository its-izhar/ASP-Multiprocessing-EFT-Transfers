/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: transfProg.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T05:34:05-05:00
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

// Process Data
typedef struct processData {
  int64_t processID;                             // Each process has it's own ID
  workerQueue_t EFTRequests;                // Each process has it's own queue
  bankAccountPool_t *accountPool;           // Each process has access to common account pool
} processData_t;

// Functions for managing processes
int64_t spawnProcesses(processData_t **processDataPool, \
  bankAccountPool_t *accountPool, int64_t NumberOfProcesses);
void askProcessesToExit(processData_t **processData, int64_t NumberOfProcesses, \
  int64_t lastAssignedID);

#endif
