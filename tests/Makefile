# This Makefile requires that GTEST_DIR is set to the directory of GoogleTest.
# GoogleTest: https://code.google.com/p/googletest/downloads/list

TESTS = pfabric_queue_bank_unittest

# Emulation stuff
CXXDEFINES += -DNO_DPDK
CXXDEFINES += -DEMULATION_ALGO
CXXDEFINES += -DFASTPASS_CONTROLLER
CXXDEFINES += -DALGO_N_CORES=1

CXXINCLUDES = 
CXXINCLUDES += -I$(PWD)/../../../../fastpass-public/src/graph-algo
CXXINCLUDES += -I$(PWD)/../../../../fastpass-public/src/arbiter
CXXINCLUDES += -I.

CXXFLAGS = $(CXXDEFINES) $(CXXINCLUDES)


# GoogleTest stuff

CPPFLAGS += -isystem $(GTEST_DIR)/include
CXXFLAGS += -g -Wall -Wextra -pthread
# too many unused functions in individual tests
CXXFLAGS += -Wno-unused-function

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.
all : $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

pfabric_queue_bank_unittest.o : ./pfabric_queue_bank_unittest.cc \
                     $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c pfabric_queue_bank_unittest.cc

pfabric_queue_bank_unittest : pfabric_queue_bank_unittest.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
