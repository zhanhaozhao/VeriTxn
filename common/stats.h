#ifndef STATS_H_
#define STATS_H_

#include "stdint.h"
#include "config.h"

class Stats_thd {
public:
	void init(uint64_t thd_id);
	void clear();

	char _pad2[CL_SIZE];
	uint64_t txn_cnt;
	uint64_t abort_cnt;
	double run_time;
	double time_man;
	double time_index;
	double time_wait;
	double time_abort;
	double time_cleanup;
	uint64_t time_ts_alloc;
	double time_query;
	double time_cache;
	uint64_t wait_cnt;
	uint64_t debug1;
	uint64_t debug2;
	uint64_t debug3;
	uint64_t debug4;
	uint64_t debug5;
	
	uint64_t latency;
	uint64_t * all_debug1;
	uint64_t * all_debug2;
	char _pad[CL_SIZE];
};

class Stats_tmp {
public:
	void init() {
		clear();
	}

	void clear() {	
		time_man = 0;
		time_index = 0;
		time_wait = 0;
	}
	double time_man;
	double time_index;
	double time_wait;
	char _pad[CL_SIZE - sizeof(double)*3];
};

class Stats {
public:
	// PER THREAD statistics
	Stats_thd ** _stats;
	// stats are first written to tmp_stats, if the txn successfully commits, 
	// copy the values in tmp_stats to _stats
	Stats_tmp ** tmp_stats;
	uint64_t * begin_ts;
    uint64_t freshness;
    uint64_t freshness_cnt;
    uint64_t version_chain_length;
    uint64_t access_cnt;
    double time_recover;

    // GLOBAL statistics
	double dl_detect_time;
	double dl_wait_time;
	uint64_t cycle_detect;
	uint64_t deadlock;	

	void init();
	void init(uint64_t thread_id);
	void clear(uint64_t tid);
	void add_debug(uint64_t thd_id, uint64_t value, uint32_t select);
	void commit(uint64_t thd_id);
	uint64_t get_min_begin_ts() {
	    uint64_t res = 0;
	    for (int i=0;i<THREAD_CNT;i++) {
	        auto tmp = begin_ts[i];
	        if (tmp != 0 && tmp > res) {
	            res = tmp;
	        }
	    }
        return res;
	}
	inline void abort(uint64_t thd_id) {
		if (STATS_ENABLE) 
			tmp_stats[thd_id]->init();
	}
	void print();
	void print_lat_distr();
};

#endif