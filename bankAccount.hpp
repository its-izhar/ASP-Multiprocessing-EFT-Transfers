/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T16:56:55-05:00
* @Email:  izharits@gmail.com
* @Filename: bankAccount.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-04T18:39:18-05:00
* @License: MIT
*/



#ifndef __BANK_ACCOUNT__
#define __BANK_ACCOUNT__


#include <pthread.h>
#include <stdint.h>

// -- Typedefs --
typedef class bankAccount bankAccount_t;
typedef class bankAccountNode node_t;
typedef class bankAccountNode* poolHandle_t;
typedef class bankAccountPool bankAccountPool_t;

// -- Class --
class bankAccount
{
private:
  int64_t number;     // Account Number
  int64_t balance;    // Balance
  pthread_mutexattr_t attr; // Attribute for mutex
  pthread_mutex_t mutex;    // Mutexcle to protect read/write access to the acc

public:
  bankAccount();                    // Default Constructor
  ~bankAccount();                   // Destructor
  int64_t lock();                                   // Lock the access to mutex
  int64_t trylock();                                // Lock the access to mutex
  int64_t unlock();                                 // releases the access to mutex
  int64_t getAccountNumber();                       // retrieves account number
  int64_t getBalance();                             // retrieves account balance
  void setBalance(int64_t newBalance);              // sets account balance
  void setAccountNumber(int64_t accountNumber);     // sets the account number
};


// -- Pool of bank accounts --
class bankAccountPool
{
private:
  poolHandle_t handle;
  void *poolMemory;
  int64_t totalAccounts;

public:
  bankAccountPool();
  ~bankAccountPool();
  void initPool(int64_t NumberOfAccounts);                    // Initialized the pool
  poolHandle_t getPoolHandle();                               // get the handle to the pool
  int64_t getTotalAccounts();                                 // Total accounts in the pool
  bankAccount_t* at(int64_t accountNumber);                   // retrieve bank account
  void addAccount(int64_t accountNumber, int64_t balance);    // Add new account to pool
  void dbgPrintAccountPool();                                 // prints all the contents of account pool
};


#endif
