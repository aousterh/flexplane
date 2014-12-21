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
%_wrap.o: %_wrap.cc
	$(CXX) $(CXXFLAGS) -c $< -fPIC -I /usr/include/python2.7/ -o $@
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.pic.o: %.cc
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

# Dependency rules for non-file targets
.PHONY: clean
all: emulation py
clean:
	rm -f emulation *.o *~ _fastemu.so fastemu.py fastemu.pyc fastemu_wrap.cc

# Dependency rules for file targets
emulation: emulation_test.o emulation.o endpoint_group.o drop_tail.o router.o \
			drivers/RouterDriver.o drivers/EndpointDriver.o
	$(CXX) $^ -o $@ $(LDFLAGS)

####################
### PYTHON WRAPPER
.PHONY: py
py: _fastemu.so

WRAP_HEADERS = \
	packet.h \
	api.h \
	api_impl.h \
	router.h \
	composite.h \
	classifiers/TorClassifier.h \
	classifiers/PyClassifier.h \
	queue_managers/PyQueueManager.h \
	schedulers/SingleQueueScheduler.h \
	schedulers/PyScheduler.h
	

%_wrap.cc: %.i $(WRAP_HEADERS) 
	swig -c++ -python -I$(RTE_SDK)/$(RTE_TARGET)/include -o $@ $< 


_fastemu.so: fastemu_wrap.o emulation.pic.o endpoint_group.pic.o router.pic.o \
			drop_tail.pic.o	drivers/EndpointDriver.pic.o \
			drivers/RouterDriver.pic.o
	$(CXX) $^ -o $@ $(LDFLAGS) -shared

