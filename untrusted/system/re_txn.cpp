// #include "global_enc.h"
#include "global_struct.h"
#include "logger.h"
// #include "global_enc.h"
// #include "index_btree_enc.h"

#include "re_txn.h"
// #include "row_enc.h"
// #include "wl.h"
// #include "ycsb.h"
// #include "common/thread.h"
// #include "common/mem_alloc.h"
// #include "occ.h"
#include "table.h"
#include "catalog.h"
#include "mem_helper.h"
// #include "index_btree.h"
// #include "index_hash.h"
// #include "index_enc.h"
// #include "logger_enc.h"

#include "api.h"

void re_txn_man::init(RPCThread * h_thd, workload * h_wl, uint64_t thd_id) {
	this->h_thd = h_thd;
	this->h_wl = h_wl;
	pthread_mutex_init(&txn_lock, NULL);
	lock_ready = false;
	ready_part = 0;
	row_cnt = 0;
	wr_cnt = 0;
	insert_cnt = 0;
	// accesses = (Access **) aligned_alloc(64, sizeof(Access *) * MAX_ROW_PER_TXN);
	// accesses = (Access **) malloc(sizeof(Access *) * MAX_ROW_PER_TXN);
	// for (int i = 0; i < MAX_ROW_PER_TXN; i++)
	// 	accesses[i] = NULL;
	num_accesses_alloc = 0;
#if CC_ALG == TICTOC || CC_ALG == SILO
	_pre_abort = (g_params["pre_abort"] == "true");
	if (g_params["validation_lock"] == "no-wait")
		_validation_no_wait = true;
	else if (g_params["validation_lock"] == "waiting")
		_validation_no_wait = false;
	else 
		assert(false);
#endif
#if CC_ALG == TICTOC
	_max_wts = 0;
	_write_copy_ptr = (g_params["write_copy_form"] == "ptr");
	_atomic_timestamp = (g_params["atomic_timestamp"] == "true");
#elif CC_ALG == SILO
	_cur_tid = 0;
#endif

	_log_entry = new char [g_max_log_entry_size]; //g_max_log_entry_size
	_log_entry_size = 0;

}

void re_txn_man::set_txn_id(txnid_t txn_id) {
	this->txn_id = txn_id;
}

txnid_t re_txn_man::get_txn_id() {
	return this->txn_id;
}

workload * re_txn_man::get_wl() {
	return h_wl;
}

uint64_t re_txn_man::get_thd_id() {
	return h_thd->get_thd_id();
}

void re_txn_man::set_ts(ts_t timestamp) {
	this->timestamp = timestamp;
}

ts_t re_txn_man::get_ts() {
	return this->timestamp;
}

void re_txn_man::insert_row(row_t * row, table_t * table) {
	if (CC_ALG == HSTORE)
		return;
	assert(insert_cnt < MAX_ROW_PER_TXN);
	insert_rows[insert_cnt ++] = row;
}

void re_txn_man::recover() {
	char default_entry[g_max_log_entry_size]; // g_max_log_entry_size
	// right now, only a single thread does the recovery job.
	if (get_thd_id() > 0)
		return;
	uint32_t count = 0;
	uint64_t txn_cnt = 0;

	while (true) {
		char * entry = default_entry;
		uint64_t tt = get_sys_clock();
		uint64_t lsn = logger->get_next_log_entry_non_atom(entry);
		if (entry == NULL) {
			if (logger->iseof()) {
				lsn = logger->get_next_log_entry_non_atom(entry);
				if (entry == NULL) {
					// std::cout<< "emtry null2"<<std::endl;
					continue;
				}
			} else { 
				PAUSE //usleep(50);
				// INC_INT_STATS(time_wait_io, get_sys_clock() - tt);
				continue;
			}
		}

		uint64_t tt2 = get_sys_clock();
		// INC_INT_STATS(time_wait_io, tt2 - tt);
		// Format for serial logging
		// | checksum | size | ... |
		assert(*(uint32_t*)entry == 0xbeef || entry[0] == 0x7f);

    	recover_txn(entry + sizeof(uint32_t) * 2);		

		// printf("size=%d lsn=%ld\n", *(uint32_t*)(entry+4), lsn);
		COMPILER_BARRIER
		//INC_INT_STATS(time_recover_txn, get_sys_clock() - tt2);
		logger->set_gc_lsn(lsn, get_thd_id());
		INC_STATS(get_thd_id(), txn_cnt, 1);
		count ++;
	}
	std::cout<< "total recover txn:" << count << std::endl;
}
