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
#include "config.h"
#include "../protocol/flags.h"
//#include "../protocol/topology.h"
#include "util/make_ring.h"
#include "packet.h"
#include "api.h"
#include "api_impl.h"
#include "router.h"
#include "endpoint_group.h"
#include "composite.h"
#include "emulation.h"
#include "emulation_impl.h"
#include "output.h"
#include "admitted.h"
#include "queue_bank.h"
#include "routing_tables/TorRoutingTable.h"
#include "routing_tables/PyRoutingTable.h"
#include "classifiers/SingleQueueClassifier.h"
#include "classifiers/FlowIDClassifier.h"
#include "classifiers/PyClassifier.h"
#include "queue_managers/PyQueueManager.h"
#include "schedulers/SingleQueueScheduler.h"
#include "schedulers/PriorityScheduler.h"
#include "schedulers/PyScheduler.h"
#include "drivers/SingleRackNetworkDriver.h"

#include "queue_managers/drop_tail.h"
#include "queue_managers/red.h"
#include "queue_managers/dctcp.h"
#include "simple_endpoint.h"
%}

#define __attribute__(x)

%include "config.h"
%include "../protocol/flags.h"
//%include "../protocol/topology.h"

%include "packet.h"
%pointer_functions(struct emu_packet, pkt)

%include "util/make_ring.h"
%include "api.h"
%include "router.h"
%include "endpoint_group.h"
%include "composite.h"

%include "emulation.h"
%include "output.h"
%include "admitted.h"
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
%include "schedulers/PriorityScheduler.h"
%feature("director") PyScheduler;
%include "schedulers/PyScheduler.h"

/** Drivers */
%include "drivers/SingleRackNetworkDriver.h"


/** Composite Routers */
%template(PyRouter) CompositeRouter<PyRoutingTable, PyClassifier, PyQueueManager, PyScheduler>;

%template(DropTailRouterBase) CompositeRouter<TorRoutingTable, FlowIDClassifier, DropTailQueueManager, SingleQueueScheduler>;
%include "queue_managers/drop_tail.h"

%template(REDRouterBase) CompositeRouter<TorRoutingTable, SingleQueueClassifier, REDQueueManager, SingleQueueScheduler>;
%include "queue_managers/red.h"

%template(DCTCPRouterBase) CompositeRouter<TorRoutingTable, SingleQueueClassifier, DCTCPQueueManager, SingleQueueScheduler>;
%include "queue_managers/dctcp.h"


/** Composite Endpoint Groups */
%template(SimpleEndpointGroupBase) CompositeEndpointGroup<SingleQueueClassifier, DropTailQueueManager, SingleQueueScheduler, SimpleSink>;
%include "simple_endpoint.h"

/** accessors */
%inline %{
struct fp_ring *get_new_pkts_ring(struct emu_state *state) {
	return state->comm_state.q_epg_new_pkts[0];
}

struct emu_admitted_traffic *get_admitted(struct emu_state *state) {
	struct emu_admitted_traffic *admitted;
	if (fp_ring_dequeue(state->q_admitted_out, (void **) &admitted) != 0)
		return NULL; /* empty */
	else
		return admitted;
}

struct emu_admitted_edge admitted_get_edge(struct emu_admitted_traffic *admitted,
			uint32_t index) {
	return admitted->edges[index];
}
%}


/** PySink */
%feature("director") PySink;
%inline %{
class PySink : public Sink {
public:
	virtual ~PySink() {}
	virtual void handle(struct emu_packet *pkt) {throw std::runtime_error("not implemented");}
};
%}

%template(PyEndpointGroupBase) CompositeEndpointGroup<PyClassifier, PyQueueManager, PyScheduler, PySink>;
%inline %{
class PyEndpointGroup : public CompositeEndpointGroup<PyClassifier, PyQueueManager, PyScheduler, PySink> {
public:
	PyEndpointGroup(PyClassifier *cla, PyQueueManager *qm, PyScheduler *sch, PySink *sink,
			uint32_t first_endpoint_id, uint32_t n_endpoints)
		: CompositeEndpointGroup<PyClassifier, PyQueueManager, PyScheduler, PySink>(cla,qm,sch,sink,first_endpoint_id, n_endpoints)
	{} 
	virtual void reset(uint16_t id) { throw std::runtime_error("not implemented");}
};
%}

