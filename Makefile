

CC = g++
OPT = -g
WARN = -Wall
INCLUDE = -Iinclude/
CFLAGS = $(OPT) $(WARN) $(INCLUDE)

# List corresponding compiled object files here (.o files)
SIM_OBJ = obj/sim_pipe.o

TESTCASES = src/testcase1 src/testcase2 src/testcase3 src/testcase4 src/testcase5 src/testcase6

#################################
# adding tmp
# default rule
all:
	$(TESTCASES)

# generic rule for converting any .cc file to any .o file
.cc.o:
	$(CC) $(CFLAGS) -c *.cc

#rule for creating the object files for all the testcases in the "testcases" folder
testcase:
	$(MAKE) -C testcases

# rules for making testcases
testcase1: .cc.o testcase
	$(CC) -o exec/testcase1 $(CFLAGS) $(SIM_OBJ) obj/testcase1.o

testcase2: .cc.o testcase
	$(CC) -o exec/testcase2 $(CFLAGS) $(SIM_OBJ) obj/testcase2.o

testcase3: .cc.o testcase
	$(CC) -o exec/testcase3 $(CFLAGS) $(SIM_OBJ) obj/testcase3.o

testcase4: .cc.o testcase
	$(CC) -o exec/testcase4 $(CFLAGS) $(SIM_OBJ) obj/testcase4.o

testcase5: .cc.o testcase
	$(CC) -o exec/testcase5 $(CFLAGS) $(SIM_OBJ) obj/testcase5.o

testcase6: .cc.o testcase
	$(CC) -o exec/testcase6 $(CFLAGS) $(SIM_OBJ) obj/testcase6.o

# type "make clean" to remove all .o files plus the sim binary
clean:
	rm -f obj/*.o
	rm -f exec/*
