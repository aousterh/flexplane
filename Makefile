# Macros
CXX = g++

CXXDEFINES =
CXXDEFINES += -DNO_DPDK
CXXDEFINES += -DEMULATION_ALGO
#CXXDEFINES += -DEMU_NO_BATCH_CALLS
CXXDEFINES += -DFASTPASS_CONTROLLER

CXXINCLUDES = 
CXXINCLUDES += -I$(PWD)/../../../fastpass-public/src/graph-algo
CXXINCLUDES += -I$(PWD)/../../../fastpass-public/src/arbiter
CXXINCLUDES += -I.

CXXFLAGS = $(CXXDEFINES) $(CXXINCLUDES)

CXXFLAGS += -g
#CXXFLAGS += -DNDEBUG
#CXXFLAGS += -DCONFIG_IP_FASTPASS_DEBUG
#CXXFLAGS += -O3
#CXXFLAGS += -O1
#CXXFLAGS += -O0
#CXXFLAGS += -debug inline-debug-info
LDFLAGS = -lm
#LDFLAGS = -debug inline-debug-info

# add more flags for swig if on Mac
UNAME_S := $(shell uname -s)
SWIG_FLAGS = -shared
ifeq ($(UNAME_S), Darwin)
	SWIG_FLAGS += -lpython -dynamiclib
endif

# Pattern rule
%_wrap.o: %_wrap.cc
	$(CXX) $(CXXFLAGS) -c $< -fPIC -I /usr/include/python2.7/ -o $@
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.drv.o: drivers/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.qm.o: queue_managers/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.pic.o: %.cc
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

# Dependency rules for non-file targets
.PHONY: clean
all: emulation py
clean:
	rm -f emulation *.o drivers/*.o queue_managers/*.o *~ _fastemu.so fastemu.py fastemu.pyc fastemu_wrap.cc

# Dependency rules for file targets
emulation: emulation_test.o emulation.o endpoint_group.o simple_endpoint.o \
			router.o \
			drop_tail.qm.o red.qm.o dctcp.qm.o hull.qm.o probdrop.qm.o \
			RouterDriver.drv.o EndpointDriver.drv.o
	$(CXX) $^ -o $@ $(LDFLAGS)

####################
### PYTHON WRAPPER
.PHONY: py
py: _fastemu.so

WRAP_HEADERS = \
	packet.h \
	router.h \
	composite.h \
	classifiers/SingleQueueClassifier.h \
	classifiers/PyClassifier.h \
	classifiers/BySourceClassifier.h \
	queue_managers/PyQueueManager.h \
	schedulers/SingleQueueScheduler.h \
	schedulers/RRScheduler.h \
	schedulers/PriorityScheduler.h \
	schedulers/PyScheduler.h

%_wrap.cc: %.i $(WRAP_HEADERS) 
	swig -c++ -python $(CXXDEFINES) $(CXXINCLUDES) -o $@ $< 


_fastemu.so: fastemu_wrap.o emulation.pic.o endpoint_group.pic.o router.pic.o \
			queue_managers/drop_tail.pic.o	\
			queue_managers/red.pic.o \
			queue_managers/dctcp.pic.o \
			queue_managers/hull.pic.o \
			queue_managers/probdrop.pic.o \
			simple_endpoint.pic.o \
			drivers/EndpointDriver.pic.o \
			drivers/RouterDriver.pic.o drivers/SingleRackNetworkDriver.pic.o
	$(CXX) $^ -o $@ $(LDFLAGS) $(SWIG_FLAGS)


