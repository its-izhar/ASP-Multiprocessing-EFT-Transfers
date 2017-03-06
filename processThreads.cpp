/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:51:43-05:00
* @Email:  izharits@gmail.com
* @Filename: processThreads.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T05:00:18-05:00
* @License: MIT
*/



#include <iostream>
#include <memory>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>

#include "debugMacros.hpp"
#include "transfProg.hpp"

using namespace std;


// Thread function (EFT requests processor)
void EFTWorker(threadData_t *data)
{
  threadData_t *workerData = data;
  EFTRequest_t requestToProcess;

  dbg_trace("PID: " << getpid() << " , " << "PPID: " << getppid() << " , " \
  << "After Spawning: threadData[" << workerData->threadID << "]: " << workerData << " , "\
  << "[Thread-ID: " << workerData->threadID << "]: "\
  << "Queue-ID: " << workerData->EFTRequests.getWorkerID());

  // workerData->accountPool->dbgPrintAccountPool();

  while(1)
  {
    // Read data from worker queue/buffer
    // This has been implemented in an atomic way with the use of
    // synchronization constructs
    requestToProcess = workerData->EFTRequests.popRequest();

    int64_t fromBalance = 0, toBalance = 0;
    int64_t fromAccount = requestToProcess.fromAccount;
    int64_t toAccount = requestToProcess.toAccount;
    int64_t transferAmount = requestToProcess.transferAmount;

    // Check if we are done
    if(fromAccount == -1 || toAccount == -1){
      break;
    }
    /*dbg_trace("[requestToProcess]: "\
    << "From: " << fromAccount << " , "\
    << "To: " << toAccount << " , "\
    << "Transfer: " << transferAmount);*/

    // -- Process the request with "restricted order" of accounts to avoid deadlocks
    // ========== ENTER Critical Section ==========
      if(fromAccount < toAccount)
      { // 1. From, 2. To
        workerData->accountPool->at(fromAccount)->lock();
        workerData->accountPool->at(toAccount)->lock();
      }
      else
      { // 1. To, 2. From
        workerData->accountPool->at(toAccount)->lock();
        workerData->accountPool->at(fromAccount)->lock();
      }
        // -- Get the balance
        fromBalance = workerData->accountPool->at(fromAccount)->getBalance();
        toBalance = workerData->accountPool->at(toAccount)->getBalance();

        /*dbg_trace("[beforeProcess]: "\
        << "From: " << fromBalance << " , "\
        << "To: " << toBalance);*/

        // -- Update the account with new balance
        workerData->accountPool->at(fromAccount)->setBalance(fromBalance - transferAmount);
        workerData->accountPool->at(toAccount)->setBalance(toBalance + transferAmount);

        /*dbg_trace("[AfterProcess]: "\
        << "From: " << workerData->accountPool->at(fromAccount)->getBalance() << " , "\
        << "To: " << workerData->accountPool->at(toAccount)->getBalance());*/

      if(fromAccount < toAccount)
      { // 1. To, 2. From
        workerData->accountPool->at(toAccount)->unlock();
        workerData->accountPool->at(fromAccount)->unlock();
      }
      else
      { // 1. From, 2. To
        workerData->accountPool->at(fromAccount)->unlock();
        workerData->accountPool->at(toAccount)->unlock();
      }
    // ========= EXIT Critical Section =========
  }
  dbg_trace("PROCESS: " << workerData->threadID << " - " << getpid() << " EXIT!");
  return;
}


// Function to create thread data and spawn threads
int64_t spawnThreads(threadData_t **threadDataPool, \
  bankAccountPool_t *accountPool, int64_t NumberOfThreads)
{
  threadData_t **threadPool = threadDataPool;
  bool spawnThreadsStatus = FAIL;
  int64_t thread = 0;

  for(thread = 0; thread < NumberOfThreads; thread++)
  {
    threadPool[thread]->threadID = thread;
    threadPool[thread]->EFTRequests.init();
    threadPool[thread]->EFTRequests.setWorkerID(thread);
    threadPool[thread]->accountPool = accountPool;

    // Spwan it
    int64_t status = fork();
    if(status < 0){
      print_output("Failed to create process: " << thread);
      exit(1);
    }
    else if(status == 0)        // Child process
    {
      // Execute worker
      EFTWorker(threadPool[thread]);

      // unmap the memory here
      munmap(threadPool[thread]->accountPool, sizeof(bankAccountPool_t));
      munmap(threadPool[thread], sizeof(threadData_t));

      // exit from child
      exit(EXIT_SUCCESS);
    }
  }
  if(thread == NumberOfThreads){
    spawnThreadsStatus = SUCCESS;
  }
  return spawnThreadsStatus;
}



// Ask threads to terminate
void askThreadsToExit(threadData_t **threadData, int64_t NumberOfThreads, \
  int64_t lastAssignedID)
{
  int64_t assignID = lastAssignedID;
  int64_t requestCount = 0;

  // Sanity checks
  if(lastAssignedID == -1 || NumberOfThreads < 0){
    return;
  }

  do {
      // Calculate worker ID to be assigned
      // Since we will be assigning the jobs in round robin fashion,
      // we will mod the result with NumberOfThreads
      assignID = (assignID + 1) % NumberOfThreads;
      ++requestCount;

      assert(threadData[assignID]->threadID == assignID);    // Sanity checks
      assert(threadData[assignID]->threadID \
        == threadData[assignID]->EFTRequests.getWorkerID());

      // Request to terminate
      threadData[assignID]->EFTRequests.requestToExit();
  } while(assignID != lastAssignedID);

  dbg_trace("Total Threads Requested to Exit: " << requestCount);
}
