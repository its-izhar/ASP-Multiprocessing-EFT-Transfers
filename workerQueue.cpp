/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T16:51:00-05:00
* @Email:  izharits@gmail.com
* @Filename: workerQueue.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T04:51:07-05:00
* @License: MIT
*/



#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "debugMacros.hpp"
#include "transfProg.hpp"
#include "workerQueue.hpp"


// Std namespace
using namespace std;

// ------------------------ Class: workerQueue ------------------------------

// Constructor
void workerQueue :: init()
{
  if(this->is_initialized == true){
    return;
  }
  this->is_initialized = true;

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
void workerQueue :: destroy()
{
  if(this->is_initialized == false){
    return;
  }
  this->is_initialized = false;

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
