%module fastpass

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "carrays.i"

namespace std {
   %template(vectorstr) vector<string>;
};

%{
#include "main.h"
#include "comm_core.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "path_sel_core.h"
#include "log_core.h"
#include "stress_test_core.h"
%}

%include "main.h"
%include "comm_core.h"
%include "admission_core.h"
%include "admission_core_common.h"
%include "path_sel_core.h"
%include "log_core.h"
%include "stress_test_core.h"
