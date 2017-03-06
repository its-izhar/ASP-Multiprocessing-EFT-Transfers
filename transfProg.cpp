/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-02T18:38:30-05:00
* @Email:  izharits@gmail.com
* @Filename: transfProg.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-06T05:02:31-05:00
* @License: MIT
*/



#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "debugMacros.hpp"
#include "transfProg.hpp"


// Std namespace
using namespace std;

// To save the order in which accounts are listed
std::vector<int64_t> accountList;

/* Parse the input file into bank account pool and EFT requests pool */
static int64_t assignWorkers(const char *fileName, threadData_t **threadData, \
  bankAccountPool_t *accountPool, int64_t NumberOfThreads, int64_t &requestCount)
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

    // Process spawn logic
    static bool poolInitDone = false;
    if(poolInitDone == false){
      poolInitDone = true;
      // InitPoolSpace here
      int64_t maxAccounts = atoi(line);
      if(maxAccounts < 1){
        dbg_trace("Error! First line should be max number of accounts");
        exit(1);
      }
      accountPool->initPool(maxAccounts);

      // Spawn processes
      bool status = spawnThreads(threadData, accountPool, NumberOfThreads);
      if(status == FAIL){
        dbg_trace("Failed to create threads!");
        return 0;
      }
      // clear and repeat the sequence
      dbg_trace("*** POOL INIT DONE! THIS SHOULD NEVER PRINT AGAIN!!! ****");
      goto CLEAR;
    }

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
      accountPool->addAccount(accountNumber, initBalance);

      /*dbg_trace("POOL: \
      Account Number: " << accountPool->at(accountNumber)->getAccountNumber() \
      << " , " << \
      "Init Balance: " << accountPool->at(accountNumber)->getBalance());*/
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

      assert(threadData[assignID]->threadID == assignID);    // Sanity checks
      assert(threadData[assignID]->threadID \
        == threadData[assignID]->EFTRequests.getWorkerID());

      // Create new EFT request
      EFTRequest_t newRequest;
      newRequest.workerID = assignID;
      newRequest.fromAccount = fromAccount;
      newRequest.toAccount = toAccount;
      newRequest.transferAmount = transferAmount;

      // Start writing;
      // NOTE:: this is data-race safe since the workerQueue class implements
      // safe IPC using mutex and condition varibales
      threadData[assignID]->EFTRequests.pushRequest(&newRequest);

      /*dbg_trace("[Thread ID: " << threadData[assignID]->threadID << ","\
      << "Job Assigned ID: " << assignID << ","\
      << "Queue ID: " << threadData[assignID]->EFTRequests.getWorkerID() << "]");*/
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
    askThreadsToExit(threadData, NumberOfThreads, assignID);
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


/* display account pool */
static void displayAccountPool(bankAccountPool_t *accountPool)
{
  accountPool->dbgPrintAccountPool();
}


/* Print the account and their balances to stdout */
static void printAccounts(bankAccountPool_t *accountPool)
{
  std::vector<int64_t>::iterator i;
  for(i = accountList.begin(); i != accountList.end(); ++i)
  {
    print_output(*i << " " << accountPool->at((int64_t)(*i))->getBalance());
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

  // If everything is fine, map the shared memory and spawn workers
  void *sAccontPool = mmap(NULL, sizeof(bankAccountPool_t), PROT_READ | PROT_WRITE, \
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if(sAccontPool == MAP_FAILED){
    print_output("(main()) PID: " << getpid() << " , " \
      "Failed to map the memory for bankAccountPool! Exiting!");
    exit(1);
  }
  // Pool of bank accounts
  bankAccountPool_t *accountPool = (bankAccountPool_t *) sAccontPool;

  // map processData memory here
  void *sProcessData[workerThreads];
  threadData_t *threadData[workerThreads];

  for(int i = 0; i < workerThreads; i++)
  {
    sProcessData[i] = mmap(NULL, sizeof(threadData_t), \
    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(sProcessData[i] == MAP_FAILED){
      print_output("(main()) PID: " << getpid() << " , " \
      "Failed to map the memory for processData: " << i << " *ABORT*");
      exit(1);
    }

    // Assign it to our threadDataPool
    threadData[i] = (threadData_t *) sProcessData[i];
  }

  // Keep the EFT Transfer Request count
  int64_t EFTRequestsCount = 0;

  // And parse the file
  int64_t parseStatus = assignWorkers(argv[1], threadData, accountPool, \
    workerThreads, EFTRequestsCount);
  if(parseStatus == FAIL)
  {
    print_output("ERROR: Failed during parsing!");
    return 0;
  }

  // wait for threads to finish
  int pStatus = 0;
  for(int64_t i=0; i<workerThreads; i++)
  {
    wait(&pStatus);
    if(WIFEXITED(pStatus)){
      dbg_trace("PROCESS: " << i << " TERMINATED!");
    }
    else if(pStatus == -1){
      dbg_trace("Error! PROCESS: " << i << " Terminaton not successful!");
    }
    // free up the worker resources
    threadData[i]->EFTRequests.destroy();
  }

  // Display the Accounts and their Balances after transfer
  displayAccountPool(accountPool);
  printAccounts(accountPool);

  // destroy the accountPool
  accountPool->deInitPool();

  // Unmap the memory
  int ustatus = munmap(accountPool, sizeof(bankAccountPool_t));
  if(ustatus != 0){
    print_output("(main()) PID: " << getpid() << " , " \
                 "Failed to unmap the accountPool memory! Exiting!");
    exit(1);
  }

  for(int i = 0; i < workerThreads; i++){
    ustatus = munmap(threadData[i], sizeof(threadData_t));
    if(ustatus != 0){
      print_output("(main()) PID: " << getpid() << " , " \
      "Failed to unmap the threadData: " << i << " memory! Exiting!");
      exit(1);
    }
  }

  return 0;
}
