/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T16:55:48-05:00
* @Email:  izharits@gmail.com
* @Filename: bankAccount.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T04:50:12-05:00
* @License: MIT
*/



#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "debugMacros.hpp"
#include "transfProg.hpp"


// Std namespace
using namespace std;

// ------------------------ Class: bankAccount ------------------------------

// Default Constructor
void bankAccount :: init()
{
  if(this->is_initialized == true){
    return;
  }
  this->is_initialized = true;

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
void bankAccount :: destroy(){
  // Cleanup
  if(this->is_initialized == false){
    return;
  }
  this->is_initialized = false;

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

// sets the balance of the acount
void bankAccount :: setBalance(int64_t newBalance){
  // update the balance
  this->balance = newBalance;
}

// sets the balance of the acount
void bankAccount :: setAccountNumber(int64_t accountNumber){
  // update the account number
  this->number = accountNumber;
}
