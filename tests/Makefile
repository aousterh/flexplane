# This Makefile requires that GTEST_DIR is set to the directory of GoogleTest.
# GoogleTest: https://code.google.com/p/googletest/downloads/list

EMU_FILES = emulation.cc emulation_core.cc router.cc endpoint_group.cc \
	simple_endpoint.cc
DRV_FILES = RouterDriver.cc EndpointDriver.cc
QM_FILES = drop_tail.cc red.cc dctcp.cc pfabric_qm.cc drop_tail_tso.cc lstf_qm.cc
SCHED_FILES = hull_sched.cc

# Rules for constructing paths to .o files
EMULATION_O = $(addprefix ../, $(addsuffix .o, $(basename $(EMU_FILES))))
DRIVER_O = $(addprefix ../drivers/, $(addsuffix .o, $(basename $(DRV_FILES))))
QM_O = $(addprefix ../queue_managers/, $(addsuffix .o, $(basename $(QM_FILES))))
SCHED_O = $(addprefix ../schedulers/, $(addsuffix .o, $(basename $(SCHED_FILES))))

EMULATION_ALL_O = $(EMULATION_O) $(DRIVER_O) $(QM_O) $(SCHED_O)
$(warning $(EMULATION_ALL_O))

# Emulation stuff
CXXDEFINES += -DNO_DPDK
CXXDEFINES += -DEMULATION_ALGO
CXXDEFINES += -DFASTPASS_CONTROLLER
CXXDEFINES += -DALGO_N_CORES=1

# Target-specific includes
tso_unittests: CXXDEFINES += -DUSE_TSO
pfabric_unittests: CXXDEFINES += -DPFABRIC
lstf_unittests: CXXDEFINES += -DLSTF

CXXINCLUDES = 
CXXINCLUDES += -I$(PWD)/../../../../fastpass-public/src/graph-algo
CXXINCLUDES += -I$(PWD)/../../../../fastpass-public/src/arbiter
CXXINCLUDES += -I..

CXXFLAGS = $(CXXDEFINES) $(CXXINCLUDES)


# GoogleTest stuff

CPPFLAGS += -isystem $(GTEST_DIR)/include
CXXFLAGS += -g -pthread
CXXFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-unused-function

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.
all : unittests round_robin_unittets lstf_unittests

clean :
	rm -f unittests round_robin_unittests gtest.a gtest_main.a *.o ../*.o \
	../drivers/*.o ../queue_managers/*.o ../schedulers/*.o lstf_unittests

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# Rules for emulation files
%.o : %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

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

# Build all unittests in this directory. Link with gtest_main.a, because we do
# not define our own Makefile.

%_unittest.o : ./%_unittest.cc \
                     $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

TESTS_CC = $(wildcard *_unittest.cc)
TESTS_O = $(addsuffix .o, $(basename $(TESTS_CC)))

unittests : $(TESTS_O) $(EMULATION_ALL_O) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

pfabric_unittests : pfabric_queue_bank_unittest.o pfabric_unittest.o $(EMULATION_ALL_O) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

round_robin_unittests : round_robin_unittest.o $(EMULATION_ALL_O) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

tso_unittests : drop_tail_tso_unittest.o $(EMULATION_ALL_O) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

lstf_unittests : lstf_queue_bank_unittest.o lstf_unittest.o $(EMULATION_ALL_O) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
