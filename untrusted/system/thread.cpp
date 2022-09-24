#include <sched.h>
#include "global.h"
#include "manager.h"
#include "thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "plock.h"
#include "occ.h"
#include "vll.h"
#include "ycsb_query.h"
#include "tpcc_query.h"
#include "mem_alloc.h"
#include "test.h"

#include "global_enc.h"
#include "thread_enc.h"

void thread_t::init(uint64_t thd_id, workload * workload) {
	_thd_id = thd_id;
	_wl = workload;
	srand48_r((_thd_id + 1) * get_sys_clock(), &buffer);
	_abort_buffer_size = ABORT_BUFFER_SIZE;
	_abort_buffer = (AbortBufferEntry *) _mm_malloc(sizeof(AbortBufferEntry) * _abort_buffer_size, 64); 
	for (int i = 0; i < _abort_buffer_size; i++)
		_abort_buffer[i].query = NULL;
	_abort_buffer_empty_slots = _abort_buffer_size;
	_abort_buffer_enable = (g_params["abort_buffer_enable"] == "true");
}

uint64_t thread_t::get_thd_id() { return _thd_id; }
uint64_t thread_t::get_host_cid() {	return _host_cid; }
void thread_t::set_host_cid(uint64_t cid) { _host_cid = cid; }
uint64_t thread_t::get_cur_cid() { return _cur_cid; }
void thread_t::set_cur_cid(uint64_t cid) {_cur_cid = cid; }

RC thread_t::run() {

// Get a thread number
#if !NOGRAPHITE
	_thd_id = CarbonGetTileId();
#endif
	if (warmup_finish) {
		mem_allocator.register_thread(_thd_id);
	}

	pthread_barrier_wait( &warmup_bar );
	stats.init(get_thd_id());
	pthread_barrier_wait( &warmup_bar );

	set_affinity(get_thd_id());

	myrand rdm;
	rdm.init(get_thd_id());
	RC rc = RCOK;


	run_txn_in_enc(this);
	// txn_man * m_txn;
	// rc = _wl->get_txn_man(m_txn, this);
	// assert (rc == RCOK);
	// glob_manager->set_txn_man(m_txn);

	UInt64 txn_cnt = 0;

	while (true) {
		starttime = get_sys_clock();

		rc = run_txn_in_enc2(this, this->get_next_ts());

		if (rc == Abort) {
			uint64_t penalty = 0;
			if (ABORT_PENALTY != 0)  {
				double r;
				drand48_r(&buffer, &r);
				penalty = r * ABORT_PENALTY;
			}
			if (!_abort_buffer_enable)
				usleep(penalty / 1000);
			else {
				assert(_abort_buffer_empty_slots > 0);
				for (int i = 0; i < _abort_buffer_size; i ++) {
					if (_abort_buffer[i].query == NULL) {
						_abort_buffer[i].query = m_query;
						_abort_buffer[i].ready_time = get_sys_clock() + penalty;
						_abort_buffer_empty_slots --;
						break;
					}
				}
			}
		}

		ts_t endtime = get_sys_clock();
		uint64_t timespan = endtime - starttime;
		INC_STATS(get_thd_id(), run_time, timespan);
		INC_STATS(get_thd_id(), latency, timespan);
		//stats.add_lat(get_thd_id(), timespan);
		if (rc == RCOK) {
			INC_STATS(get_thd_id(), txn_cnt, 1);
			stats.commit(get_thd_id());
			txn_cnt ++;
		} else if (rc == Abort) {
			INC_STATS(get_thd_id(), time_abort, timespan);
			INC_STATS(get_thd_id(), abort_cnt, 1);
			stats.abort(get_thd_id());
			// m_txn->abort_cnt ++;
		}

		if (rc == FINISH)
			return rc;
		if (!warmup_finish && txn_cnt >= WARMUP / g_thread_cnt) 
		{
			stats.clear( get_thd_id() );
			return FINISH;
		}

		if (warmup_finish && txn_cnt >= MAX_TXN_PER_PART) {
			assert(txn_cnt == MAX_TXN_PER_PART);
	        if( !ATOM_CAS(_wl->sim_done, false, true) )
				assert( _wl->sim_done);
	    }
	    if (_wl->sim_done) {
   		    return FINISH;
   		}
	}
	assert(false);
}

void
thread_t::generate_txn_for_run(base_query *& m_query) {
	// generate a txn request with m_query
	if (WORKLOAD != TEST) {
		int trial = 0;
		if (_abort_buffer_enable) {
			m_query = NULL;
			while (trial < 2) {
				ts_t curr_time = get_sys_clock();
				ts_t min_ready_time = UINT64_MAX;
				if (_abort_buffer_empty_slots < _abort_buffer_size) {
					for (int i = 0; i < _abort_buffer_size; i++) {
						if (_abort_buffer[i].query != NULL && curr_time > _abort_buffer[i].ready_time) {
							m_query = _abort_buffer[i].query;
							_abort_buffer[i].query = NULL;
							_abort_buffer_empty_slots ++;
							break;
						} else if (_abort_buffer_empty_slots == 0 
									&& _abort_buffer[i].ready_time < min_ready_time) 
							min_ready_time = _abort_buffer[i].ready_time;
					}
				}
				if (m_query == NULL && _abort_buffer_empty_slots == 0) {
					assert(trial == 0);
					M_ASSERT(min_ready_time >= curr_time, "min_ready_time=%ld, curr_time=%ld\n", min_ready_time, curr_time);
					usleep((min_ready_time - curr_time) / 1000);
				}
				else if (m_query == NULL) {
					m_query = query_queue->get_next_query( _thd_id );
				#if CC_ALG == WAIT_DIE
					m_txn->set_ts(get_next_ts());
				#endif
				}
				if (m_query != NULL)
					break;
			}
		} else {
			// if (rc == RCOK)
			m_query = query_queue->get_next_query( _thd_id );
		}
	}
	this->m_query = m_query;
	INC_STATS(_thd_id, time_query, get_sys_clock() - starttime);
}


ts_t
thread_t::get_next_ts() {
	if (g_ts_batch_alloc) {
		if (_curr_ts % g_ts_batch_num == 0) {
			_curr_ts = glob_manager->get_ts(get_thd_id());
			_curr_ts ++;
		} else {
			_curr_ts ++;
		}
		return _curr_ts - 1;
	} else {
		_curr_ts = glob_manager->get_ts(get_thd_id());
		return _curr_ts;
	}
}

// RC thread_t::runTest(txn_man * txn)
// {
// 	RC rc = RCOK;
// 	if (g_test_case == READ_WRITE) {
// 		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
// #if CC_ALG == OCC
// 		txn->start_ts = get_next_ts(); 
// #endif
// 		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 1);
// 		printf("READ_WRITE TEST PASSED\n");
// 		return FINISH;
// 	}
// 	else if (g_test_case == CONFLICT) {
// 		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
// 		if (rc == RCOK)
// 			return FINISH;
// 		else 
// 			return rc;
// 	}
// 	assert(false);
// 	return RCOK;
// }
