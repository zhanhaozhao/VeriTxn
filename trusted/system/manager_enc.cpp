#include "manager_enc.h"
#include "global_enc.h"

// #include "common/helper.h"
#include "mem_helper_enc.h"

// #include "row.h"
// #include "txn.h"
// #include "pthread.h"
// #include "common/stats.h"


#include "api.h"

void ManagerEnc::init() {
	// timestamp = (uint64_t *) aligned_alloc(64, sizeof(uint64_t));
	timestamp = (uint64_t *) malloc(sizeof(uint64_t));
	*timestamp = 1;
	_last_min_ts_time = 0;
	_min_ts = 0;
	// _epoch = (uint64_t *) aligned_alloc(64, sizeof(uint64_t));
	_epoch = (uint64_t *) malloc(sizeof(uint64_t));
	// _last_epoch_update_time = (ts_t *) aligned_alloc(64, sizeof(uint64_t));
	_last_epoch_update_time = (ts_t *) malloc(sizeof(uint64_t));
	_epoch = 0;
	_last_epoch_update_time = 0;
	// all_ts = (ts_t volatile **) aligned_alloc(64, sizeof(ts_t *) * g_thread_cnt_enc);
	all_ts = (ts_t volatile **) malloc(sizeof(ts_t *) * g_thread_cnt_enc);
	for (uint32_t i = 0; i < g_thread_cnt_enc; i++) {
		// all_ts[i] = (ts_t *) aligned_alloc(64, sizeof(ts_t));
		all_ts[i] = (ts_t *) malloc(sizeof(ts_t));
	}

	_all_txns = new txn_man * [g_thread_cnt_enc];
	for (UInt32 i = 0; i < g_thread_cnt_enc; i++) {
		*all_ts[i] = UINT64_MAX;
		_all_txns[i] = nullptr;
	}
	_thd_txn_ids = new uint64_t [g_thread_cnt_enc];
	for (UInt32 i = 0; i < g_thread_cnt_enc; i++) {
		_thd_txn_ids[i] = 0;
	}
	for (UInt32 i = 0; i < BUCKET_CNT; i++)
		pthread_mutex_init( &mutexes[i], NULL );
}

uint64_t 
ManagerEnc::get_ts(uint64_t thread_id) {
	if (g_ts_batch_alloc_enc)
		assert(g_ts_alloc_enc == TS_CAS);
	uint64_t time;
//	uint64_t starttime = get_cur_time_ocall();
	switch(g_ts_alloc_enc) {
	case TS_MUTEX :
		pthread_mutex_lock( &ts_mutex );
		time = ++(*timestamp);
		pthread_mutex_unlock( &ts_mutex );
		break;
	case TS_CAS :
		if (g_ts_batch_alloc_enc)
			time = ATOM_FETCH_ADD((*timestamp), g_ts_batch_num_enc);
		else 
			time = ATOM_FETCH_ADD((*timestamp), 1);
		break;
	case TS_HW :
// #ifndef NOGRAPHITE
// 		time = CarbonGetTimestamp();
// #else
		assert(false);
// #endif
		break;
	case TS_CLOCK :
        assert(false);
//		time = get_cur_time_ocall() * g_thread_cnt_enc + thread_id;
		break;
	default :
		assert(false);
	}
//	INC_STATS_ENC(thread_id, time_ts_alloc, get_cur_time_ocall() - starttime);
	return time;
}

ts_t ManagerEnc::get_min_ts(uint64_t tid) {
    assert(false);
//	uint64_t now = get_cur_time_ocall();
//	uint64_t last_time = _last_min_ts_time;
//	if (tid == 0 && now - last_time > MIN_TS_INTVL)
//	{
//		ts_t min = UINT64_MAX;
//    		for (UInt32 i = 0; i < g_thread_cnt_enc; i++)
//			if (*all_ts[i] < min)
//		    	    	min = *all_ts[i];
//		if (min > _min_ts)
//			_min_ts = min;
//	}
//	return _min_ts;
}

void ManagerEnc::add_ts(uint64_t thd_id, ts_t ts) {
	assert( ts >= *all_ts[thd_id] || 
		*all_ts[thd_id] == UINT64_MAX);
	*all_ts[thd_id] = ts;
}

void ManagerEnc::set_txn_man(txn_man * txn) {
	int thd_id = txn->get_thd_id();
	_all_txns[thd_id] = txn;
}


uint64_t ManagerEnc::hash(row_t * row) {
	uint64_t addr = (uint64_t)row / MEM_ALLIGN;
    return (addr * 1103515247 + 12345) % BUCKET_CNT;
}
 
void ManagerEnc::lock_row(row_t * row) {
	int bid = hash(row);
	pthread_mutex_lock( &mutexes[bid] );	
}

void ManagerEnc::release_row(row_t * row) {
	int bid = hash(row);
	pthread_mutex_unlock( &mutexes[bid] );
}
	
void
ManagerEnc::update_epoch()
{
    assert(false);
//	ts_t time = get_cur_time_ocall();
//	if (time - *_last_epoch_update_time > LOG_BATCH_TIME * 1000 * 1000) {
//		*_epoch = *_epoch + 1;
//		*_last_epoch_update_time = time;
//	}
}
