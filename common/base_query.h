#ifndef _BASE_QUERY_H_
#define _BASE_QUERY_H_

// #include "global.h"
// #include "helper.h"

// class workload;
// class ycsb_query;
// class tpcc_query;

// #include "global_common.h"
#include "wl.h"

class base_query {
public:
	virtual void init(uint64_t thd_id, workload * h_wl) = 0;
	uint64_t waiting_time;
	uint64_t part_num;
	uint64_t * part_to_access;
};

#endif