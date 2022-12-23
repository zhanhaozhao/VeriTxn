#ifndef MANAGER_H_
#define MANAGER_H_


#include "common/global_common.h"
#include "common/base_row.h"
// #include "global.h"
// #include "global_enc.h"
#include "base_row.h"
#include "re_txn.h"
// class base_row_t;
// class txn_man;

class Manager {
public:
	void 			init();
	// returns the next timestamp.
	ts_t			get_ts(uint64_t thread_id);

	// For MVCC. To calculate the min active ts in the system
	void 			add_ts(uint64_t thd_id, ts_t ts);
	ts_t 			get_min_ts(uint64_t tid = 0);

	// HACK! the following mutexes are used to model a centralized
	// lock/timestamp manager. 
 	void 			lock_row(base_row_t * row);
	void 			release_row(base_row_t * row);
	
	re_txn_man * 		get_txn_man(int thd_id) { return _all_txns[thd_id]; };
	void 			set_txn_man(re_txn_man * txn);

	uint64_t 		get_thd_txn_id(int thd_id) { return _thd_txn_ids[thd_id]; };
	void 			set_thd_txn_id(int thd_id) { _thd_txn_ids[thd_id]++; };

	uint64_t 		get_epoch() { return *_epoch; };
	void 	 		update_epoch();
private:
	// for SILO
	volatile uint64_t * _epoch;		
	ts_t * 			_last_epoch_update_time;

	pthread_mutex_t ts_mutex;
	uint64_t *		timestamp;
	pthread_mutex_t mutexes[BUCKET_CNT];
	uint64_t 		hash(base_row_t * row);
	ts_t volatile * volatile * volatile all_ts;
	re_txn_man ** 		_all_txns;
	uint64_t *		_thd_txn_ids;
	// for MVCC 
	volatile ts_t	_last_min_ts_time;
	ts_t			_min_ts;
	
};

#endif