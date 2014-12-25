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
#include "classifiers/TorClassifier.h"
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

/** Classifiers */
%apply uint32_t *OUTPUT {uint32_t *port, uint32_t *queue};
%include "classifiers/TorClassifier.h"
%clear uint32_t *port;	
%clear uint32_t *queue;

/** Queue Managers */
%feature("director") PyQueueManager;
%include "queue_managers/PyQueueManager.h"

/** Schedulers */
%include "schedulers/SingleQueueScheduler.h"
%feature("director") PyScheduler;
%include "schedulers/PyScheduler.h"


/*********************
 * PyClassifier
 *********************/
%feature("director") PyClassifier;

%feature("shadow") PyClassifier::classify(struct emu_packet *pkt, 
		uint32_t *port,	uint32_t *queue)
 %{
	def classify(self, pkt):
		raise RuntimeError("implement classify method (returns (port, queue))")
%}

%feature("shadow") PyClassifier::py_classify(struct emu_packet *pkt, 
		uint32_t *port,	uint32_t *queue)
 %{
	def py_classify(self, pkt, port_ptr, queue_ptr):
		port, queue = self.classify(pkt)
		u32_assign(port_ptr, port)
		u32_assign(queue_ptr, queue)
%}

%include "classifiers/PyClassifier.h"


/** Composite Routers */
%template(PyCompositeRouter) CompositeRouter<PyClassifier, PyQueueManager, PyScheduler>;
%template(REDRouterBase) CompositeRouter<TorClassifier, REDQueueManager, SingleQueueScheduler>;

%include "endpoint.h"
%include "endpoint_group.h"
%template(DropTailRouterBase) CompositeRouter<TorClassifier, DropTailQueueManager, SingleQueueScheduler>;
%include "drop_tail.h"
%include "red.h"
