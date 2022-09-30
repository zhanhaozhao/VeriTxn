#ifndef QUERY_H_
#define QUERY_H_

// #include "global.h"
// #include "helper.h"

#include "global_common.h"
#include "base_query.h"
#include "wl.h"

// class workload;
// class ycsb_query;
// class tpcc_query;


// #include "ycsb_query.h"
// #include "tpcc_query.h"



// class base_query {
// public:
// 	virtual void init(uint64_t thd_id, workload * h_wl) = 0;
// 	uint64_t waiting_time;
// 	uint64_t part_num;
// 	uint64_t * part_to_access;
// };

// All the querise for a particular thread.

// TODO we assume a separate task queue for each thread in order to avoid 
// contention in a centralized query queue. In reality, more sofisticated 
// queue model might be implemented.
class Query_queue {
public:
	void init(workload * h_wl);
	void init_per_thread(int thread_id);
	base_query * get_next_query(uint64_t thd_id); 
	
private:
	static void * threadInitQuery(void * This);

	Query_thd ** all_queries;
	workload * _wl;
	static int _next_tid;
};

#endif