%module(directors="1") fastemu

%include "stdint.i"
%include "cpointer.i"
%include "exception.i"
%include "typemaps.i"

%{
#include <exception>
%}

%exception {
	try {
        $function
	} catch(const std::exception& err) {
        SWIG_exception(SWIG_RuntimeError, err.what());
	} catch(...) {
        SWIG_exception(SWIG_RuntimeError, "Unknown exception");
	}
}

%pointer_functions(uint32_t, u32)


%{
#include "util/make_ring.h"
#include "packet.h"
#include "api.h"
#include "api_impl.h"
#include "router.h"
#include "composite.h"
#include "emulation.h"
#include "emulation_impl.h"
#include "output.h"
#include "queue_bank.h"
#include "routing_tables/TorRoutingTable.h"
#include "routing_tables/PyRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "classifiers/FlowIDClassifier.h"
#include "classifiers/PyClassifier.h"
#include "queue_managers/PyQueueManager.h"
#include "red.h"
#include "schedulers/SingleQueueScheduler.h"
#include "schedulers/PyScheduler.h"

#include "endpoint.h"
#include "endpoint_group.h"
#include "drop_tail.h"
%}

#define __attribute__(x)

%include "packet.h"
%pointer_functions(struct emu_packet, pkt)

%include "util/make_ring.h"
%include "api.h"
%include "router.h"
%include "composite.h"

%include "emulation.h"
%include "output.h"
%include "queue_bank.h"
%template(PacketQueueBank) QueueBank<struct emu_packet>;

/** Routing Tables */
%include "routing_tables/TorRoutingTable.h"
%feature("director") PyRoutingTable;
%include "routing_tables/PyRoutingTable.h"


/** Classifiers */
%include "classifiers/SingleQueueClassifier.h"
%include "classifiers/FlowIDClassifier.h"
%feature("director") PyClassifier;
%include "classifiers/PyClassifier.h"

/** Queue Managers */
%feature("director") PyQueueManager;
%include "queue_managers/PyQueueManager.h"

/** Schedulers */
%include "schedulers/SingleQueueScheduler.h"
%feature("director") PyScheduler;
%include "schedulers/PyScheduler.h"


/** Composite Routers */
%template(PyCompositeRouter) CompositeRouter<PyRoutingTable, PyClassifier, PyQueueManager, PyScheduler>;
%template(REDRouterBase) CompositeRouter<TorRoutingTable, SingleQueueClassifier, REDQueueManager, SingleQueueScheduler>;

%include "endpoint.h"
%include "endpoint_group.h"
%template(DropTailRouterBase) CompositeRouter<TorRoutingTable, FlowIDClassifier, DropTailQueueManager, SingleQueueScheduler>;
%include "drop_tail.h"
%include "red.h"
