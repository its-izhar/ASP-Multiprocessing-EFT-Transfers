/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: transfProg.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-02T23:58:33-05:00
* @License: MIT
*/



#ifndef __EFT_TRANSFER__
#define __EFT_TRANSFER__

#include <map>
#include <vector>
#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include "debugMacros.hpp"


#define           LINE_BUFFER             50
#define           MAX_WORKERS             10000


// Classes
typedef class bankAccount
{
private:
  int64_t number;     // Account Number
  int64_t balance;    // Balance
  pthread_mutexattr_t attr; // Attribute for mutex
  pthread_mutex_t mutex;    // Mutexcle to protect read/write access to the acc

public:
  bankAccount();                    // Default Constructor
  bankAccount(int64_t acc, int64_t bal);    // Constructor`
  ~bankAccount();                   // Destructor
  int64_t lock();                       // Lock the access to mutex
  int64_t trylock();                    // Lock the access to mutex
  int64_t unlock();                     // releases the access to mutex
  int64_t getAccountNumber();           // retrieves account number
  int64_t getBalance();                 // retrieves account balance
  void setBalance(int64_t newBalance);  // sets account balance
} bankAccount_t;

// Item for work queue
typedef struct EFTRequest {
  int64_t workerID;
  int64_t fromAccount;
  int64_t toAccount;
  int64_t transferAmount;
} EFTRequest_t;

// FIFO queue for each worker
typedef class workerQueue
{
private:
  int64_t workerID;
  std::queue<EFTRequest_t*> Queue;          // Queue to hold jobs
  bool shouldExit;                          // flag to indicate termination
  sem_t goodToRead;                         // Sem to indicate worker to proceed
  sem_t mutex;                              // mutex to protect the queue access

public:
  workerQueue();                            // Constructor
  ~workerQueue();                           // Destructor
  int64_t getWorkerID();                        // retrieves the worker ID
  void setWorkerID(int64_t ID);                 // sets worker ID
  void pushRequest(EFTRequest_t *request);  // Adds the item from the the back
  EFTRequest_t *popRequest();               // removes the item from the front
  void requestToExit();                     // request the worker to terminate
} workerQueue_t;

// Typedefs
typedef std::map<int64_t, bankAccount_t> bankAccountPool_t;
typedef bankAccountPool_t::iterator bankAccountIterator_t;

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
