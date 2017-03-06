# @Author: Izhar Shaikh
# @Date:   2017-02-15T20:05:45-05:00
# @Email:  izharits@gmail.com
# @Filename: test_output.py
# @Last modified by:   Izhar Shaikh
# @Last modified time: 2017-02-17T19:25:35-05:00

import sys

fileToOpen = open(sys.argv[1], "rb")

line = ' '
accountList = dict()
accountListInOrder = []
fromAccount = 0
toAccount = 0
transferAmount = 0
count = 0

while True:
    line = fileToOpen.readline()
    if line == '':
        break
    if line[0] != 'T':
        account, balance = (int(i) for i in line.split())
        accountList[account] = balance
        count = count + 1
        accountListInOrder.append(account)
        #print account, balance
    else:
        _, fr, to, tr = line.split()
        fromAccount, toAccount, transferAmount = int(fr), int(to), int(tr)
        accountList[fromAccount] -= transferAmount
        accountList[toAccount] += transferAmount
        #print fromAccount, toAccount, transferAmount

for account in accountListInOrder:
    print account, accountList[account]

fileToOpen.close()
