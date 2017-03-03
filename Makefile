# @Author: Izhar Shaikh <izhar>
# @Date:   2017-03-02T18:51:25-05:00
# @Email:  izharits@gmail.com
# @Filename: Makefile
# @Last modified by:   izhar
# @Last modified time: 2017-03-02T18:52:44-05:00
# @License: MIT



# Add the new TARGETS here
TARGETS = transfProg
CC = g++
HEADERS = -I.
CFLAGS = -Wall -Werror -std=c++11 -pthread -Og
DEBUG_FLAGS = -g -DDEBUG
SOURCES = transfProg.cpp classMethods.cpp processThreads.cpp

all: clean $(TARGETS)

transfProg:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(HEADERS) -o $@ $(SOURCES)

clean:
	rm -rf $(TARGETS) *.o
