#ifndef DB_THREAD_H_
#define DB_THREAD_H_


// #include "global.h"
// #include "txn.h"
// #include "global_common.h"
// #include "wl.h"
#include "stdlib.h"
#include "base_query.h"
#include <random>
#include "thread.h"

// class workload;
// class base_query;

class thread_t : Thread {
public:
	// uint64_t _thd_id;
	workload * _wl;
	ts_t starttime;
	base_query * m_query;
	#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
	Logqueue * enc_log_queue;
	#endif

	// uint64_t 	get_thd_id();
	uint64_t	get_thd_id() { return _thd_id; }

	uint64_t 	get_host_cid();
	void 	 	set_host_cid(uint64_t cid);

	uint64_t 	get_cur_cid();
	void 		set_cur_cid(uint64_t cid);

	void 		init(uint64_t thd_id, uint64_t node_id, workload * workload);
	// the following function must be in the form void* (*)(void*)
	// to run with pthread.
	// conversion is done within the function.
	RC 			run();
	RC			run_enc();
	void 		setup();
	void 		send_init_done_to_all_nodes();
	void		generate_txn_for_run(base_query *& m_query);

private:
	uint64_t 	_host_cid;
	uint64_t 	_cur_cid;
	ts_t 		_curr_ts;

	ts_t 		get_next_ts();

	// RC	 		runTest(txn_man * txn);
	// drand48_data buffer;
	std::default_random_engine dre;
	std::uniform_real_distribution<double> uid;


	// A restart buffer for aborted txns.
	struct AbortBufferEntry	{
		ts_t ready_time;
		base_query * query;
	};
	AbortBufferEntry * _abort_buffer;
	int _abort_buffer_size;
	int _abort_buffer_empty_slots;
	bool _abort_buffer_enable;
};

#endif