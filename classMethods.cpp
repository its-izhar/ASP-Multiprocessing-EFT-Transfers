/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: classMethods.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-03T03:49:40-05:00
* @License: MIT
*/



#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include "debugMacros.hpp"
#include "transfProg.hpp"

using namespace std;


// ------------------------ Class: bankAccount ------------------------------
// Constructor
bankAccount :: bankAccount(int64_t accountNumber, int64_t initBalance){
  this->number = accountNumber;
  this->balance = initBalance;

  // Attribute for mutex
  bool mutexAttrStatus = pthread_mutexattr_init(&this->attr);
  if(mutexAttrStatus != 0){
    print_output("Mutex Attribute init failed: "\
    << "Acc: " << accountNumber << " , "\
    << "Balance: " << initBalance);
    exit(1);
  }
  // Allow mutex to be shared among processes
  bool mutexSharedStatus = pthread_mutexattr_setpshared(&this->attr, \
    PTHREAD_PROCESS_SHARED);
  if(mutexSharedStatus != 0){
    print_output("Mutex PTHREAD_PROCESS_SHARED Attribute init failed: "\
    << "Acc: " << accountNumber << " , "\
    << "Balance: " << initBalance);
    exit(1);
  }
  // Init mutex with specified attributes
  bool mutexStatus = pthread_mutex_init(&this->mutex, &this->attr);
  if(mutexStatus != 0){
    print_output("Mutex init failed: "\
    << "Acc: " << accountNumber << " , "\
    << "Balance: " << initBalance);
    exit(1);
  }
}

// Default Constructor
bankAccount :: bankAccount() {
  this->number = -1;
  this->balance = -1;

  // Attribute for mutex
  bool mutexAttrStatus = pthread_mutexattr_init(&this->attr);
  if(mutexAttrStatus != 0){
    print_output("Mutex Attribute init failed!");
    exit(1);
  }
  // Allow mutex to be shared among processes
  bool mutexSharedStatus = pthread_mutexattr_setpshared(&this->attr, \
    PTHREAD_PROCESS_SHARED);
  if(mutexSharedStatus != 0){
    print_output("Mutex PTHREAD_PROCESS_SHARED Attribute init failed!");
    exit(1);
  }
  // Init mutex with specified attributes
  bool mutexStatus = pthread_mutex_init(&this->mutex, &this->attr);
  if(mutexStatus != 0){
    print_output("Mutex init failed!");
    exit(1);
  }
}

// Destructor
bankAccount :: ~bankAccount(){
  // Cleanup
  pthread_mutexattr_destroy(&this->attr);
  pthread_mutex_destroy(&this->mutex);
}

// locks the account access
int64_t bankAccount :: lock(){
  // lock mutex
  return pthread_mutex_lock(&this->mutex);
}

// try to lock the account access; returns otherwise
int64_t bankAccount :: trylock(){
  // try to lock mutex
  return pthread_mutex_trylock(&this->mutex);
}

// releases the account access to the account
int64_t bankAccount :: unlock(){
  // unlock mutex
  return pthread_mutex_unlock(&this->mutex);
}

// retrieves account balance
int64_t bankAccount :: getBalance(){
  // get the current balance
  return this->balance;
}

// retrieves account number
int64_t bankAccount :: getAccountNumber(){
  // get the current balance
  return this->number;
}

// Destructor
void bankAccount :: setBalance(int64_t newBalance){
  // update the balance
  this->balance = newBalance;
}


// ------------------------ Class: workerQueue ------------------------------
// Constructor
workerQueue :: workerQueue(){
  this->workerID = -1;
  this->shouldExit = false;

  // Setup buffer
  this->buffer.in = 0;
  this->buffer.out = 0;
  this->buffer.capacity = MAX_WORKER_BUFFERSIZE;
  memset(this->buffer.items, 0, sizeof(this->buffer.items));

  // Process shared
  bool semStatus = sem_init(&this->items, 1, 0);   // Init "items" sem to 0
  if(semStatus != 0){
    print_output("Sem init failed! Worker ID: " << workerID);
    exit(1);
  }
  // Init "spaces" sem to size of the buffer
  semStatus = sem_init(&this->spaces, 1, this->buffer.capacity);
  if(semStatus != 0){
    print_output("Sem init failed! Worker ID: " << workerID);
    exit(1);
  }
  // Process shared
  bool mutexStatus = sem_init(&this->mutex, 1, 1);     // Init sem to 1
  if(mutexStatus != 0){
    print_output("Sem init failed! Worker ID: " << workerID);
    exit(1);
  }
}

// Destructor
workerQueue :: ~workerQueue(){
  // Cleanup
  sem_destroy(&this->mutex);
  sem_destroy(&this->items);
  sem_destroy(&this->spaces);
}

// retrieves workerQueue ID
int64_t workerQueue :: getWorkerID(){
  return this->workerID;
}

// sets worker queue ID
void workerQueue :: setWorkerID(int64_t ID){
  this->workerID = ID;
}

// Requests the worker to terminate
void workerQueue :: requestToExit()
{
  sem_wait(&this->mutex);
  // -- CRITICAL Start
    if(this->shouldExit == true){
      sem_post(&this->mutex);
      return;
    }
    this->shouldExit = true;
    sem_post(&this->items);              // Indicate that worker should terminate
  // -- CRITICAL End
  sem_post(&this->mutex);
}

// Adds a new request at the from the back of the queue
void workerQueue :: pushRequest(EFTRequest_t *newRequest)
{
  sem_wait(&this->spaces);              // Indicate we we want to occupy a space

  // -- CRITICAL Start
  sem_wait(&this->mutex);
    // Add new request to the queue
    memset(&this->buffer.items[this->buffer.in], 0, sizeof(EFTRequest_t));
    memcpy(&this->buffer.items[this->buffer.in], newRequest, sizeof(EFTRequest_t));
    // Increment buffer index
    this->buffer.in = (this->buffer.in + 1) % this->buffer.capacity;
  sem_post(&this->mutex);

  // -- CRITICAL End
  sem_post(&this->items);              // Indicate that the request can be read
}

// Removes the request from the front of the queue
EFTRequest_t workerQueue :: popRequest()
{
  EFTRequest_t request = { -1, -1, -1, -1};
  int value = -1;
  // if there are 0 items, then we will be blocked
  // else we will decrement the current no. of items
  // to Indicate that we will read it
  sem_wait(&this->items);

  // -- CRITICAL Start
  sem_wait(&this->mutex);
    sem_getvalue(&this->items, &value);
    if(value == 0 && this->shouldExit == true){
      sem_post(&this->mutex);
      return request;
    }
    // Copy the request from buffer
    memcpy(&request, &this->buffer.items[this->buffer.out], sizeof(EFTRequest_t));
    this->buffer.out = (this->buffer.out + 1) % this->buffer.capacity;
  sem_post(&this->mutex);
  // -- CRITICAL End

  sem_post(&this->spaces);        // Indicate that a space has been emptied after reading

  return request;
}
