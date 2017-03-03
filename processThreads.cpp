/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:51:43-05:00
* @Email:  izharits@gmail.com
* @Filename: processThreads.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-02T18:52:16-05:00
* @License: MIT
*/



#include <iostream>
#include <memory>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include "debugMacros.hpp"
#include "transfProg.hpp"

using namespace std;


// Thread function (EFT requests processor)
static void *EFTWorker(void *data)
{
  threadData_t *workerData = (threadData_t *) data;
  EFTRequest_t *requestToProcess = NULL;

  while((requestToProcess = workerData->EFTRequests.popRequest()) != NULL)
  {
    /*dbg_trace("[Thread-ID: " << workerData->threadID << "]: "\
    << "Queue-ID: " << workerData->EFTRequests->getWorkerID() << " , "\
    << "Queue-size: " << workerData->EFTRequests->size() << " , "\
    << "Account Pool: " << workerData->accountPool->size());*/

    int64_t fromBalance = 0, toBalance = 0;
    int64_t fromAccount = requestToProcess->fromAccount;
    int64_t toAccount = requestToProcess->toAccount;
    int64_t transferAmount = requestToProcess->transferAmount;

    /*dbg_trace("[requestToProcess]: "\
    << "From: " << fromAccount << " , "\
    << "To: " << toAccount << " , "\
    << "Transfer: " << transferAmount);*/

    // See if it is the last job
    if(fromAccount == -1 || toAccount == -1){
      delete requestToProcess;
      requestToProcess = NULL;
      break;
    }

    // -- Process the request with "restricted order" of accounts to avoid deadlocks
    // ========== ENTER Critical Section ==========
      if(fromAccount < toAccount)
      { // 1. From, 2. To
        workerData->accountPool->at(fromAccount).lock();
        workerData->accountPool->at(toAccount).lock();
      }
      else
      { // 1. To, 2. From
        workerData->accountPool->at(toAccount).lock();
        workerData->accountPool->at(fromAccount).lock();
      }
        // -- Get the balance
        fromBalance = workerData->accountPool->at(fromAccount).getBalance();
        toBalance = workerData->accountPool->at(toAccount).getBalance();

        /*dbg_trace("[beforeProcess]: "\
        << "From: " << fromBalance << " , "\
        << "To: " << toBalance);*/

        // -- Update the account with new balance
        workerData->accountPool->at(fromAccount).setBalance(fromBalance - transferAmount);
        workerData->accountPool->at(toAccount).setBalance(toBalance + transferAmount);

        /*dbg_trace("[AfterProcess]: "\
        << "From: " << workerData->accountPool->at(fromAccount).getBalance() << " , "\
        << "To: " << workerData->accountPool->at(toAccount).getBalance());*/

      if(fromAccount < toAccount)
      { // 1. To, 2. From
        workerData->accountPool->at(toAccount).unlock();
        workerData->accountPool->at(fromAccount).unlock();
      }
      else
      { // 1. From, 2. To
        workerData->accountPool->at(fromAccount).unlock();
        workerData->accountPool->at(toAccount).unlock();
      }
    // ========= EXIT Critical Section =========

    // Cleanup
    delete requestToProcess;
    requestToProcess = NULL;
  }
  dbg_trace("THREAD: " << workerData->threadID << " EXIT!");
  pthread_exit(NULL);
}


// Function to create thread data and spawn threads
int64_t spawnThreads(pthread_t *threads, threadData_t *threadDataPool, \
  bankAccountPool_t *accountPool, int64_t NumberOfThreads)
{
  threadData_t *threadPool = threadDataPool;
  pthread_t *threadID = threads;
  bool spawnThreadsStatus = FAIL;
  int64_t thread = 0;

  for(thread = 0; thread < NumberOfThreads; thread++)
  {
    threadPool[thread].threadID = thread;
    threadPool[thread].EFTRequests.setWorkerID(thread);
    threadPool[thread].accountPool = accountPool;
    // Spwan it
    int64_t status = pthread_create(&threadID[thread], NULL, &EFTWorker, (void*) &threadPool[thread]);
    if(status != 0){
      print_output("Failed to create thread: " << thread);
      exit(1);
    }
  }
  if(thread == NumberOfThreads){
    spawnThreadsStatus = SUCCESS;
  }
  return spawnThreadsStatus;
}



// Ask threads to terminate
void askThreadsToExit(threadData_t *threadData, bankAccountPool_t &accountPool,\
   int64_t NumberOfThreads, int64_t lastAssignedID)
{
  int64_t fromAccount = -1, toAccount = -1, transferAmount = 0;
  int64_t assignID = lastAssignedID;
  int64_t requestCount = 0;

  // the last job
  fromAccount = -1;
  toAccount = -1;
  transferAmount = 0;

  // Sanity checks
  if(lastAssignedID == -1 || NumberOfThreads < 0){
    return;
  }

  // This loop is added to give each worker a last job which will have
  // both the from and to account numbers as -1 and the transferAmount 0
  // The logic works irrespective of the number of threads and requests,
  // as well as who was the last worker that got assigned the job
  do {
      // Calculate worker ID to be assigned
      // Since we will be assigning the jobs in round robin fashion,
      // we will mod the result with NumberOfThreads
      assignID = (assignID + 1) % NumberOfThreads;
      ++requestCount;

      assert(threadData[assignID].threadID == assignID);    // Sanity checks
      assert(threadData[assignID].threadID \
        == threadData[assignID].EFTRequests.getWorkerID());

      // Create new EFT request
      EFTRequest_t* newRequest = new EFTRequest_t();
      newRequest->workerID = assignID;
      newRequest->fromAccount = fromAccount;
      newRequest->toAccount = toAccount;
      newRequest->transferAmount = transferAmount;

      // Start writing;
      // NOTE:: this is data-race safe since the workerQueue class implements
      // race safe mechanism to write and read from worker queue using semaphores
      threadData[assignID].EFTRequests.pushRequest(newRequest);

      /*dbg_trace("[Thread ID: " << threadData[assignID].threadID << ","\
      << "Job Assigned ID: " << assignID << ","\
      << "Queue ID: " << threadData[assignID].EFTRequests->getWorkerID() << ","\
      << "Queue Size: " << threadData[assignID].EFTRequests->size() << "]");*/

  } while(assignID != lastAssignedID);

  // dbg_trace("Total Last Jobs: " << requestCount);
}
