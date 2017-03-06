/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T16:38:49-05:00
* @Email:  izharits@gmail.com
* @Filename: workerQueue.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T04:16:15-05:00
* @License: MIT
*/


#ifndef __WORKER_QUEUE__
#define __WORKER_QUEUE__


#include <stdint.h>
#include <semaphore.h>


// Maximum size of the worker queue for each worker
#define   MAX_WORKER_BUFFERSIZE   16

// -- Typedefs --
typedef struct EFTRequest EFTRequest_t;
typedef struct EFTRequestsBuffer Buffer_t;
typedef struct workerQueue workerQueue_t;

// -- Structures --
// Item for worker queue
struct EFTRequest {
  int64_t workerID;
  int64_t fromAccount;
  int64_t toAccount;
  int64_t transferAmount;
};

// Buffer to hold many items of EFTRequest_t type
struct EFTRequestsBuffer {
  int in;
  int out;
  int64_t capacity;
  EFTRequest_t items[MAX_WORKER_BUFFERSIZE];
};

// -- Classes --
// FIFO queue for each worker
class workerQueue
{
private:
  int64_t workerID;
  sem_t spaces;                             // Sem to indicate no. of present empty spaces
  sem_t items;                              // Sem to indicate no. of present items
  sem_t mutex;                              // mutex to protect the queue access
  bool shouldExit;                          // flag to indicate termination
  Buffer_t buffer;                  // worker queue to hold EFT Requests
  bool is_initialized = false;

public:
  void init();                            // Constructor
  void destroy();                           // Destructor
  int64_t getWorkerID();                    // retrieves the worker ID
  void setWorkerID(int64_t ID);             // sets worker ID
  void pushRequest(EFTRequest_t *request);  // Adds the item from the the back
  EFTRequest_t popRequest();                // removes the item from the front
  void requestToExit();                     // request the worker to terminate
};


#endif
