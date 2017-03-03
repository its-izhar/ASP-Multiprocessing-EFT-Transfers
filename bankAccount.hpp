/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T16:56:55-05:00
* @Email:  izharits@gmail.com
* @Filename: bankAccount.hpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-03T17:07:00-05:00
* @License: MIT
*/



#ifndef __BANK_ACCOUNT__
#define __BANK_ACCOUNT__


#include <pthread.h>
#include <stdint.h>
#include <map>

// -- Typedefs --
typedef class bankAccount bankAccount_t;
typedef std::map<int64_t, bankAccount_t> bankAccountPool_t;
typedef bankAccountPool_t::iterator bankAccountIterator_t;

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
  bankAccount(int64_t acc, int64_t bal);    // Constructor`
  ~bankAccount();                   // Destructor
  int64_t lock();                       // Lock the access to mutex
  int64_t trylock();                    // Lock the access to mutex
  int64_t unlock();                     // releases the access to mutex
  int64_t getAccountNumber();           // retrieves account number
  int64_t getBalance();                 // retrieves account balance
  void setBalance(int64_t newBalance);  // sets account balance
};


#endif
