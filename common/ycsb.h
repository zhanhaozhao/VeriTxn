#ifndef YCSB_H_
#define YCSB_H_

// #include "global.h"
#include "wl.h"
// #include "txn.h"
// #include "common/helper.h"

// class ycsb_query;

class ycsb_wl : public workload {
public :
	RC init();
	RC init_table();
	RC init_schema(std::string schema_file);
	// RC get_txn_man(txn_man *& txn_manager, thread_t * h_thd);
	int key_to_part(uint64_t key) {
		uint64_t rows_per_part = SYNTH_TABLE_SIZE / PART_CNT;
		return key / rows_per_part;
	};
	INDEX * the_index;
	table_t * the_table;
private:
	void init_table_parallel();
	void * init_table_slice();
	static void * threadInitTable(void * This) {
		((ycsb_wl *)This)->init_table_slice(); 
		return NULL;
	}
	pthread_mutex_t insert_lock;
	//  For parallel initialization
	static int next_tid;
};

#endif
