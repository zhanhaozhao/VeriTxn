#ifndef BASE_QUERY_H_
#define BASE_QUERY_H_

#include "wl.h"
#include <random>

// #include "global.h"
// #include "helper.h"

// class workload;
class ycsb_query;
class tpcc_query;

// #include "global_common.h"


class base_query {
public:
	virtual void init(uint64_t thd_id, workload * h_wl) = 0;
	virtual ~base_query() = default;
	uint64_t waiting_time;
	uint64_t part_num;
	uint64_t * part_to_access;
};

class Query_thd {
public:
	void init(workload * h_wl, int thread_id);
	base_query * get_next_query(); 
	int q_idx;
#if WORKLOAD == YCSB
	ycsb_query * queries;
#else 
	tpcc_query * queries;
#endif
	char pad[CL_SIZE - sizeof(void *) - sizeof(int)];
	// drand48_data buffer;
	std::default_random_engine dre;
	std::uniform_real_distribution<double> uid;
	std::uniform_int_distribution<uint64_t> iid;
};


#endif