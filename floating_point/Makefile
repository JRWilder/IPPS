CC = g++
OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN)

# List corresponding compiled object files here (.o files)
SIM_OBJ_FP = sim_pipe_fp.o

TESTCASES = testcase_fp0 testcase_fp1 testcase_fp2 testcase_fp3 testcase_fp4 testcase_fp5

#################################

# default rule
all:	$(TESTCASES)

# generic rule for converting any .cc file to any .o file
.cc.o:
	$(CC) $(CFLAGS) -c *.cc

#rule for creating the object files for all the testcases in the "testcases" folder
testcase:
	$(MAKE) -C testcases

# rules for making testcases
testcase_fp0: .cc.o testcase
	$(CC) -o bin/testcase_fp0 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp0.o

testcase_fp1: .cc.o testcase
	$(CC) -o bin/testcase_fp1 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp1.o

testcase_fp2: .cc.o testcase
	$(CC) -o bin/testcase_fp2 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp2.o

testcase_fp3: .cc.o testcase
	$(CC) -o bin/testcase_fp3 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp3.o

testcase_fp4: .cc.o testcase
	$(CC) -o bin/testcase_fp4 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp4.o

testcase_fp5: .cc.o testcase
	$(CC) -o bin/testcase_fp5 $(CFLAGS) $(SIM_OBJ_FP) testcases/testcase_fp5.o

# type "make clean" to remove all .o files plus the sim binary
clean:
	rm -f testcases/*.o
	rm -f *.o
	rm -f bin/*
