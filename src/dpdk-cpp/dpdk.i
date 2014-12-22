%module dpdk

%include "stdint.i"


%{
#include "util/getter_setter.h"
#include "EthernetDevice.h"
#include "Ring.h"
%}

#define __attribute__(x)

%include "util/getter_setter.h"
%include "EthernetDevice.h"
%include "Ring.h"