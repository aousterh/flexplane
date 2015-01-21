%module fastpass

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "carrays.i"

namespace std {
   %template(vectorstr) vector<string>;
};

%{
extern "C" {
#include <rte_ip.h>
#include "../protocol/platform/generic.h"
#include "../graph-algo/algo_config.h"
#include "../graph-algo/admissible.h"
#include "main.h"
#include "control.h"
#include "comm_core.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "path_sel_core.h"
#include "log_core.h"
#include "stress_test_core.h"
#include "control.h"
#include "dpdk-time.h"
}
%}

#define __attribute__(x)
#define __rte_cache_aligned

#define _RTE_IP_H_ /* for platform/generic.h */
%include "../protocol/platform/generic.h"
%include "../graph-algo/algo_config.h"

%rename($ignore, %$isfunction) ""; 	// Ignore all functions
%include "../graph-algo/admissible.h"
%rename("%s") "";	// turn off whitelist mode

%include "main.h"
%include "control.h"
%include "comm_core.h"
%include "admission_core.h"
%include "admission_core_common.h"
%include "path_sel_core.h"
%include "log_core.h"
%include "stress_test_core.h"
%include "control.h"
%include "dpdk-time.h"
