/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-06T05:32:14-05:00
* @Email:  izharits@gmail.com
* @Filename: manageProcesses.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T05:35:44-05:00
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


// Worker function (EFT requests processing in a forked child process)
void EFTWorker(processData_t *data)
{
  processData_t *workerData = data;
  EFTRequest_t requestToProcess;

  dbg_trace("PID: " << getpid() << " , " << "PPID: " << getppid() << " , " \
  << "After Spawning: processData[" << workerData->processID << "]: " << workerData << " , "\
  << "[Processe-ID: " << workerData->processID << "]: "\
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
  dbg_trace("PROCESS: " << workerData->processID << " - " << getpid() << " EXIT!");
  return;
}


// Function to create process data and spawn processes
int64_t spawnProcesses(processData_t **processDataPool, \
  bankAccountPool_t *accountPool, int64_t NumberOfProcesses)
{
  processData_t **processPool = processDataPool;
  bool spawnProcessesStatus = FAIL;
  int64_t process = 0;

  for(process = 0; process < NumberOfProcesses; process++)
  {
    processPool[process]->processID = process;
    processPool[process]->EFTRequests.init();
    processPool[process]->EFTRequests.setWorkerID(process);
    processPool[process]->accountPool = accountPool;

    // Spwan it
    int64_t status = fork();
    if(status < 0){
      print_output("Failed to create process: " << process);
      exit(1);
    }
    else if(status == 0)        // Child process
    {
      // Execute worker
      EFTWorker(processPool[process]);

      // unmap the memory here
      munmap(processPool[process]->accountPool, sizeof(bankAccountPool_t));
      munmap(processPool[process], sizeof(processData_t));

      // exit from child
      exit(EXIT_SUCCESS);
    }
  }
  if(process == NumberOfProcesses){
    spawnProcessesStatus = SUCCESS;
  }
  return spawnProcessesStatus;
}



// Ask processes to terminate
void askProcessesToExit(processData_t **processData, int64_t NumberOfProcesses, \
  int64_t lastAssignedID)
{
  int64_t assignID = lastAssignedID;
  int64_t requestCount = 0;

  // Sanity checks
  if(lastAssignedID == -1 || NumberOfProcesses < 0){
    return;
  }

  do {
      // Calculate worker ID to be assigned
      // Since we will be assigning the jobs in round robin fashion,
      // we will mod the result with NumberOfProcesses
      assignID = (assignID + 1) % NumberOfProcesses;
      ++requestCount;

      assert(processData[assignID]->processID == assignID);    // Sanity checks
      assert(processData[assignID]->processID \
        == processData[assignID]->EFTRequests.getWorkerID());

      // Request to terminate
      processData[assignID]->EFTRequests.requestToExit();
  } while(assignID != lastAssignedID);

  dbg_trace("Total Processes Requested to Exit: " << requestCount);
}
