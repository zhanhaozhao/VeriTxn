#include "thread_enc.h"
#include "ycsb_txn.h"
#include "tpcc_txn.h"
#include "test_txn.h"
#include "global_enc.h"
#include "api.h"
#include "manager.h"
#include "occ.h"

void global_init() {

    glob_manager = (Manager *) _mm_malloc(sizeof(Manager), 64);
	glob_manager->init();

	if (g_cc_alg == DL_DETECT) 
		dl_detector.init();

}

void global_init2() {

#if CC_ALG == HSTORE
part_lock_man.init();
#elif CC_ALG == OCC
occ_man.init();
#elif CC_ALG == VLL
vll_man.init();
#endif
    
}

// thread_local txn_man * m_txn;

void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd) {

	m_txn = (ycsb_txn_man *)
		_mm_malloc( sizeof(ycsb_txn_man), 64 );
	new(m_txn) ycsb_txn_man();
	m_txn->init(h_thd, h_thd->_wl, h_thd->get_thd_id());

	// assert (rc == RCOK);
	glob_manager->set_txn_man(m_txn);
}

// thread_local base_query * m_query = NULL;
thread_local uint64_t thd_txn_id = 0;

RC run_txn_in_enc(thread_t * h_thd, ts_t txn_ts) {

	RC rc = RCOK;

	txn_man * m_txn;

	assert (glob_manager);
	if (glob_manager->get_txn_man(h_thd->get_thd_id())) {
		init_txn_in_enc(m_txn, h_thd);
	} else {
		m_txn = glob_manager->get_txn_man(h_thd->get_thd_id());
	}
	assert (m_txn);

	generate_txn_ocall(h_thd, h_thd->m_query);

	base_query * m_query = h_thd->m_query;

	assert (m_query);

	// generate_txn_for_run(m_query);
	// m_txn->abort_cnt = 0;
//#if CC_ALG == VLL
//		_wl->get_txn_man(m_txn, this);
//#endif

	m_txn->set_txn_id(h_thd->get_thd_id() + thd_txn_id * g_thread_cnt);
	thd_txn_id ++;

	if ((CC_ALG == HSTORE && !HSTORE_LOCAL_TS)
			|| CC_ALG == MVCC 
			|| CC_ALG == HEKATON
			|| CC_ALG == TIMESTAMP) 
		m_txn->set_ts(txn_ts);

	rc = RCOK;
#if CC_ALG == HSTORE
	if (WORKLOAD == TEST) {
		uint64_t part_to_access[1] = {0};
		rc = part_lock_man.lock(m_txn, &part_to_access[0], 1);
	} else 
		rc = part_lock_man.lock(m_txn, m_query->part_to_access, m_query->part_num);
#elif CC_ALG == VLL
	vll_man.vllMainLoop(m_txn, m_query);
#elif CC_ALG == MVCC || CC_ALG == HEKATON
	glob_manager->add_ts(get_thd_id(), m_txn->get_ts());
#elif CC_ALG == OCC
	// In the original OCC paper, start_ts only reads the current ts without advancing it.
	// But we advance the global ts here to simplify the implementation. However, the final
	// results should be the same.
	m_txn->start_ts = txn_ts; 
#endif
	if (rc == RCOK) 
	{
#if CC_ALG != VLL
		if (WORKLOAD == TEST)
			rc = runTest(m_txn);
		else 
			rc = m_txn->run_txn(m_query);
#endif
#if CC_ALG == HSTORE
		if (WORKLOAD == TEST) {
			uint64_t part_to_access[1] = {0};
			part_lock_man.unlock(m_txn, &part_to_access[0], 1);
		} else 
			part_lock_man.unlock(m_txn, m_query->part_to_access, m_query->part_num);
#endif
	}
	return rc;

}


RC runTest(txn_man * txn)
{
	RC rc = RCOK;
	if (g_test_case == READ_WRITE) {
// 		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
// #if CC_ALG == OCC
// 		txn->start_ts = get_next_ts(); 
// #endif
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 1);
		printf("READ_WRITE TEST PASSED\n");
		return FINISH;
	}
	else if (g_test_case == CONFLICT) {
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
		if (rc == RCOK)
			return FINISH;
		else 
			return rc;
	}
	assert(false);
	return RCOK;
}
