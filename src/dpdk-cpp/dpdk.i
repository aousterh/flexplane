%module dpdk

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"

namespace std {
   %template(vectorstr) vector<string>;
};

%{
#include "util/getter_setter.h"
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_pci.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <string.h>

#include "EthernetDevice.h"
#include "Ring.h"
#include "PacketPool.h"
%}

#define __attribute__(x)

%include <rte_config.h>

/** rte_eal.h */
%inline %{
int rte_eal_init(const std::vector<std::string> &args) {
#define MAX_ARGS 128	
	char *argv[MAX_ARGS+1];
	int argc = args.size(); 
	
	if (argc > MAX_ARGS)
		throw std::runtime_error("args too long, we support up to 128 args");
#undef MAX_ARGS
		
	for (int i = 0; i < argc; i++)
		argv[i] = strndup(args[i].c_str(), args[i].size());
	argv[argc] = NULL;
	
	int ret = rte_eal_init(argc, argv);
	
	/* strings in argv are used by DPDK, so we do not free. */	
}
%}

%include <rte_lcore.h>

/** rte_pci.h */
int rte_eal_pci_probe(void);
%inline %{
void rte_eal_pci_dump() {
	rte_eal_pci_dump(stdout);
}
void dump_pci_drivers() {
	struct rte_pci_driver *drv = NULL;
	TAILQ_FOREACH(drv, &pci_driver_list, next) {
		printf("driver name: %s\n", drv->name);
	}
}
%}

/** rte_ethdev.h */
uint8_t rte_eth_dev_count(void);

/** rte_log.h */
//void rte_set_log_level(uint32_t level);
//int rte_log_cur_msg_loglevel(void);
%ignore rte_logs;
%ignore eal_default_log_stream;
%ignore rte_log;
%ignore rte_vlog;
%include <rte_log.h>


%include "util/getter_setter.h"
%include "EthernetDevice.h"
%include "Ring.h"
%include "PacketPool.h"
