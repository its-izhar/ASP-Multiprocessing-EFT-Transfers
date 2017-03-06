# @Author: Izhar Shaikh
# @Date:   2017-02-15T16:28:07-05:00
# @Email:  izharits@gmail.com
# @Filename: generate_testfile.py
# @Last modified by:   Izhar Shaikh
# @Last modified time: 2017-02-15T21:18:39-05:00

import random

fileToWrite = open("testfile", 'w')

numberOfAccounts = 5000
minRange = 100000
maxRange = 5000000

numberOfTransfers = 100000
transferMinRange = 50
transferMaxRange = 1000

accountsString = ''
transferString = ''

for i in xrange (1, numberOfAccounts):
    initBalance = random.randint(minRange, maxRange)
    accountsString += (str(i) + " " + str(initBalance) + "\n")
    if i % 5000 == 0:
        fileToWrite.write(accountsString);
        accountsString = ''

fileToWrite.write(accountsString);

for j in xrange (1, numberOfTransfers):
    transferAmount = random.randint(transferMinRange, transferMaxRange)
    fromAccount = random.randint(1, numberOfAccounts-1)
    toAccount = random.randint(1, numberOfAccounts-1)
    if fromAccount == toAccount:
        if toAccount < numberOfAccounts-1:
            toAccount += 1
        else:
            toAccount -= 1
    transferString += ("Transfer " + str(fromAccount) + " " + str(toAccount) \
                       + " " + str(transferAmount) + "\n")
    if j % 10000 == 0:
        fileToWrite.write(transferString)
        transferString = ''

fileToWrite.write(transferString);

fileToWrite.close()
