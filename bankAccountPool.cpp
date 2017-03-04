/**
* @Author: Izhar Shaikh <izhar>
* @Date:   2017-03-03T19:20:49-05:00
* @Email:  izharits@gmail.com
* @Filename: bankAccountPool.cpp
* @Last modified by:   izhar
* @Last modified time: 2017-03-04T18:47:47-05:00
* @License: MIT
*/



#include "bankAccount.hpp"
#include "debugMacros.hpp"
#include <stdbool.h>
#include <assert.h>

using namespace std;


// Global varibale to hold pointer to bankAccountPool shared memory (mmap)
// This will hold the address of the next available memory block for adding
// bank account to the pool
static node_t* sPoolBlock = NULL;
// flag to hold the NumberOfAccounts in the pool
static int64_t sPoolSpace = 0;
// flag to mark that we are initalized
static bool is_initialized = false;


// This one is a struct for a bankAccount which will reside in
// a bank account pool (which is an AVL Tree)
struct bankAccountNode
{
  int height;
  bankAccount_t account;
  node_t* left;
  node_t* right;
};

// -- Initialize the account pool
static void initPoolSpace(void *blockAddr, int64_t NumberOfAccounts)
{
  if( is_initialized == false){
    // mark that we are done
    is_initialized = true;
    // Set the sPoolBlock address to point to memory mapped by main()
    sPoolBlock = (node_t*) blockAddr;
    sPoolSpace = NumberOfAccounts;
    node_t *lastAddr = sPoolBlock + NumberOfAccounts;

    dbg_trace("Account Pool Initialized at Addr: " << sPoolBlock << " , " \
              "Total Accounts: " << NumberOfAccounts << " , " \
              "Total Size: " << (float) (sizeof(node_t)*NumberOfAccounts) << " , " \
              "Each Account Size: " << sizeof(node_t) << " , " \
              "End Addr: " << lastAddr  );
  }
}

// -- Create and return a new bankAccount node to be added to the accountPool
static node_t* getNewNode(int64_t accountNumber, int64_t accountBalance)
{
  if( is_initialized == false){
    dbg_trace("bankAccountPool is not Initialized yet!");
    return NULL;
  }
  if(sPoolSpace == 0){
    dbg_trace("bankAccountPool is Full!");
    return NULL;
  }
  // Point to current head of the mapped memory
  node_t* newNode = sPoolBlock;
  newNode->height = 1;
  newNode->left = NULL;
  newNode->right = NULL;
  newNode->account.setAccountNumber(accountNumber);
  newNode->account.setBalance(accountBalance);

  // Update the head of the mapped memory and decrement the poolSpace count
  ++sPoolBlock;
  --sPoolSpace;

  if(sPoolSpace == 0){
    dbg_trace("bankAccountPool is now full. No more space!");
  }
  return newNode;
}


// A utility function to get height of the tree
static int height(node_t *N)
{
    if (N == NULL)
        return 0;
    return N->height;
}

// A utility function to get maximum of two integers
static int max(int a, int b)
{
    return (a > b)? a : b;
}


// A utility function to right rotate subtree rooted with y
// See the diagram given above.
static node_t *rightRotate(node_t *y)
{
    node_t *x = y->left;
    node_t *T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    y->height = max(height(y->left), height(y->right))+1;
    x->height = max(height(x->left), height(x->right))+1;

    // Return new root
    return x;
}

// A utility function to left rotate subtree rooted with x
// See the diagram given above.
static node_t *leftRotate(node_t *x)
{
    node_t *y = x->right;
    node_t *T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    //  Update heights
    x->height = max(height(x->left), height(x->right))+1;
    y->height = max(height(y->left), height(y->right))+1;

    // Return new root
    return y;
}

// Get Balance factor of node N
static int getBalance(node_t *N)
{
    if (N == NULL)
        return 0;
    return height(N->left) - height(N->right);
}

// Recursive function to insert key in subtree rooted
// with node and returns new root of subtree.
static node_t* insert(node_t* node, int64_t key, int64_t value)
{
    /* 1.  Perform the normal BST insertion */
    if (node == NULL)
        return(getNewNode(key, value));

    if (key < node->account.getAccountNumber())
        node->left  = insert(node->left, key, value);
    else if (key > node->account.getAccountNumber())
        node->right = insert(node->right, key, value);
    else // Equal keys are not allowed in BST
        return node;

    /* 2. Update height of this ancestor node */
    node->height = 1 + max(height(node->left),
                           height(node->right));

    /* 3. Get the balance factor of this ancestor
          node to check whether this node became
          unbalanced */
    int balance = getBalance(node);

    // If this node becomes unbalanced, then
    // there are 4 cases

    // Left Left Case
    if (balance > 1 && key < node->left->account.getAccountNumber())
        return rightRotate(node);

    // Right Right Case
    if (balance < -1 && key > node->right->account.getAccountNumber())
        return leftRotate(node);

    // Left Right Case
    if (balance > 1 && key > node->left->account.getAccountNumber())
    {
        node->left =  leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left Case
    if (balance < -1 && key < node->right->account.getAccountNumber())
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    /* return the (unchanged) node pointer */
    return node;
}



// retrieves the handle to the requested node
static node_t* getNode(node_t *node, int64_t key)
{
  if(node == NULL){
    return node;
  }
  int64_t currentKey = node->account.getAccountNumber();
  if(key < currentKey){
    return getNode(node->left, key);
  }
  else if(key > currentKey){
    return getNode(node->right, key);
  }
  else {
    // we found it, return
    return node;
  }
}

#ifdef DEBUG_TEST
// Prints account pool (Inorder)
void printAccountPool(node_t *node)
{
  if(node == NULL){
    return;
  }
  printAccountPool(node->left);
  dbg_trace("Acc Number: " << node->account.getAccountNumber() << " , "\
            "Balance: " << node->account.getBalance());
  printAccountPool(node->right);
}
#endif

// --------- bankAccountPool --------
bankAccountPool :: bankAccountPool()
{
  this->handle = NULL;
  this->poolMemory = NULL;
  this->totalAccounts = 0;
}

void bankAccountPool :: initPool(int64_t NumberOfAccounts)
{
  if( is_initialized == true ){
    return;
  }
  // mmap or malloc the memory here;
  // this will be shared among processess
  this->poolMemory = malloc(NumberOfAccounts * sizeof(node_t));
  initPoolSpace(poolMemory, NumberOfAccounts);
}

bankAccountPool :: ~bankAccountPool()
{
  free(this->poolMemory);
}

// retrieves current handle to the bankAccountPool
// (this points to the root node in AVL tree)
poolHandle_t bankAccountPool :: getPoolHandle()
{
  return this->handle;
}

// retrieves the current count of bank accounts in the pool
int64_t bankAccountPool :: getTotalAccounts()
{
  return this->totalAccounts;
}

// Inserts a new bank account to the pool
void bankAccountPool :: addAccount(int64_t accountNumber, \
  int64_t balance) {
    // Keep the current handle updated with the root node
    this->handle = insert(this->handle, accountNumber, balance);
    this->totalAccounts++;
  }

//  retrieves the handle to account requested
bankAccount_t* bankAccountPool :: at(int64_t accountNumber)
{
  poolHandle_t requestedNode = NULL;
  bankAccount_t *requestedAccount = NULL;
  // Get the requested node
  requestedNode = getNode(this->handle, accountNumber);
  // return the account handle
  requestedAccount = &requestedNode->account;
  return requestedAccount;
}

#ifdef DEBUG_TEST
// -- debug print --
void bankAccountPool :: dbgPrintAccountPool()
{
  printAccountPool(this->handle);
}
#endif
