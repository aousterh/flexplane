%module fastemu

%include "stdint.i"
%include "cpointer.i"
%include "exception.i"

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

%{
#include "packet.h"
#include "api.h"
#include "api_impl.h"
#include "router.h"
#include "composite.h"
#include "classifiers/TorClassifier.h"
#include "classifiers/VirtualClassifier.h"
%}

#define __attribute__(x)

%include "packet.h"
%include "api.h"
%include "router.h"
%include "composite.h"
%include "classifiers/TorClassifier.h"
%include "classifiers/VirtualClassifier.h"

