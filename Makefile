# Macros
CXX = g++
CXXFLAGS = -g
CXXFLAGS += -DNDEBUG
CXXFLAGS += -O3
#CXXFLAGS += -O1
CXXFLAGS += -DNO_DPDK
#CXXFLAGS += -DEMU_NO_BATCH_CALLS
#CXXFLAGS += -debug inline-debug-info
CXXFLAGS += -I$(PWD)/../../../fastpass-public/src/graph-algo
CXXFLAGS += -I$(PWD)/../../../fastpass-public/src/arbiter
CXXFLAGS += -I.
CXXFLAGS += -DEMULATION_ALGO
LDFLAGS = -lm
#LDFLAGS = -debug inline-debug-info

# Pattern rule
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

# Dependency rules for non-file targets
all: emulation
clean:
	rm -f emulation *.o *~

# Dependency rules for file targets
emulation: emulation_test.o emulation.o endpoint_group.o drop_tail.o
	$(CXX) $< emulation.o endpoint_group.o drop_tail.o -o $@ $(LDFLAGS)
