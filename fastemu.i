%module fastemu

%include "stdint.i"


%{
#include "packet.h"
#include "api.h"
#include "api_impl.h"
#include "router.h"
#include "composite.h"
#include "classifiers/TorClassifier.h"
%}

#define __attribute__(x)

%include "packet.h"
%include "api.h"
%include "router.h"
%include "composite.h"
%include "classifiers/TorClassifier.h"

