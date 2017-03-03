/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: transfProg.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-02T23:33:42-05:00
* @License: MIT
*/



#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include "debugMacros.hpp"
#include "transfProg.hpp"


using namespace std;

// Global
// To save the order in which accounts are listed
std::vector<int64_t> accountList;


/* Parse the input file into bank account pool and EFT requests pool */
static int64_t assignWorkers(const char *fileName, threadData_t *threadData, \
  bankAccountPool_t &accountPool, int64_t NumberOfThreads, int64_t &requestCount)
{
  // Input file stream & buffer
  std::ifstream fileStream;
  std::stringstream stringParser;
  char line[LINE_BUFFER] = { 0 };
  int64_t accountNumber = -1, initBalance = 0;
  int64_t fromAccount = -1, toAccount = -1, transferAmount = 0;
  std::string transferString;
  bool initDone = false;
  int64_t assignID = -1;

  // Open the fileStream
  fileStream.open(fileName, std::ifstream::in);
  if(!fileStream.is_open()){
    dbg_trace("Failed to open the file: " << fileName);
    return FAIL;
  }
  // Read from the file
  while(fileStream.good())
  {
    fileStream.getline(line, LINE_BUFFER);          // read a line
    dbg_trace("String: " << line);
    // Check if the transfer requests are coming
    if (isalpha(line[0]) && line[0]=='T' ){
      initDone = true;
    }
    stringParser.str(line);            // convert c-like string to stringParser

    // If we're not done reading accounts yet, keep reading and add to accountPool
    if(!initDone)
    {
      stringParser >> accountNumber >> initBalance;
      dbg_trace("Account Number: " \
      << accountNumber << " , " << "Init Balance: " << initBalance);

      if(accountNumber == -1){
        goto CLEAR;
      }

      // Keep the order of the accounts
      accountList.push_back(accountNumber);
      // Adding the object to the map here
      accountPool.emplace(std::make_pair(accountNumber, \
        bankAccount(accountNumber, initBalance)));

      dbg_trace("POOL: \
      Account Number: " << accountPool[accountNumber].getAccountNumber() \
      << " , " << \
      "Init Balance: " << accountPool[accountNumber].getBalance());
    }
    else
    {
      // Once we are done reading accounts; read EFT requests
      stringParser >> transferString >> fromAccount >> toAccount >> transferAmount;
      dbg_trace("From: " << fromAccount << \
      " To: " << toAccount << " Amount: " << transferAmount);

      // Stop reading once we've reached invalid accounts i.e. EOF
      if(fromAccount == -1 || toAccount == -1){
        goto CLEAR;
      }

      // Assign the job to next worker
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
      // safe IPC using mutex and condition varibales
      threadData[assignID].EFTRequests.pushRequest(newRequest);

      /*dbg_trace("[Thread ID: " << threadData[assignID].threadID << ","\
      << "Job Assigned ID: " << assignID << ","\
      << "Queue ID: " << threadData[assignID].EFTRequests.getWorkerID() << "]")*/
    }

CLEAR:
    // Clear the buffer here, before reading the next line
    memset(line, '\0', LINE_BUFFER);
    stringParser.str("");       // Clear the stringstream
    stringParser.clear();       // needed to clear the stringstream
    accountNumber = fromAccount = toAccount = -1;
    initBalance = transferAmount = 0;
  }
  // Check why we got out
  if(fileStream.eof())
  {
    dbg_trace("Reached End-of-File!");
    dbg_trace("Total Transfer Requests: " << requestCount);
    // Ask all threads to terminate
    askThreadsToExit(threadData, accountPool, NumberOfThreads, assignID);
  }
  else {
    dbg_trace("Error while reading!");
    //fileStream.close();
    //return FAIL;
  }
  // Close the fileStream
  fileStream.close();

  return SUCCESS;
}


#if 0
/* display account pool */
static void displayAccountPool(bankAccountPool_t &accountPool)
{
  bankAccountIterator_t i;
  for(i = accountPool.begin(); i != accountPool.end(); ++i)
  {
    dbg_trace("POOL:\
    Account Number: " << i->second.getAccountNumber() \
    << " , " << \
    "Balance: " << i->second.getBalance());
  }
}
#endif

/* Print the account and their balances to stdout */
static void printAccounts(bankAccountPool_t &accountPool)
{
  std::vector<int64_t>::iterator i;
  for(i = accountList.begin(); i != accountList.end(); ++i)
  {
    print_output(*i << " " << accountPool[*i].getBalance());
  }
}

// ------------------------ main() ------------------------------
int main(int argc, char const *argv[])
{
  // Check and parse the command line argument
  if(argc != 3){
    print_output("USAGE:");
    print_output("\t./transfProg <PathToInputFile> <NumberOfThreads>");
    return 0;
  }
  // Check the validity of the input file,
  int64_t fileStatus = access(argv[1], F_OK | R_OK);
  if(fileStatus != 0){
    print_output("Failed to access the input file or file doesn't exist!");
    print_output("Please check the path to the input file is correct.");
    return 0;
  }
  // Check the validity of the worker threads
  int64_t workerThreads = atoi((const char *) argv[2]);
  if(workerThreads < 1 || workerThreads > MAX_WORKERS){
    print_output("Invalid number of workers: " << workerThreads \
     << "\nEnter buffer size between 1 to " << MAX_WORKERS);
    return 0;
  }
  // If everything is fine, init threads
  bankAccountPool_t accountPool;                // Pool of bank accounts
  threadData_t threadData[workerThreads];       // thread data array
  pthread_t threads[workerThreads];             // pthreads ID array
  int64_t EFTRequestsCount = 0;

  bool status = spawnThreads(threads, threadData, &accountPool, workerThreads);
  if(status == FAIL){
    dbg_trace("Failed to create threads!");
    return 0;
  }

  // And parse the file
  int64_t parseStatus = assignWorkers(argv[1], threadData, accountPool, \
    workerThreads, EFTRequestsCount);
  if(parseStatus == FAIL)
  {
    print_output("ERROR: Failed during parsing!");
    return 0;
  }

  // wait for threads to finish
  for(int64_t i=0; i<workerThreads; i++){
    pthread_join(threads[i], NULL);
    dbg_trace("THREAD: " << i << " TERMINATED!");
  }

  // Display the Accounts and their Balances after transfer
  printAccounts(accountPool);

  return 0;
}
