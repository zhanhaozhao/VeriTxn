#include "thread_enc.h"
#include "ycsb_txn.h"
#include "tpcc_txn.h"
#include "test_txn.h"
#include "global_enc_struct.h"
#include "api.h"
// #include "manager.h"
#include "occ.h"
#include "index_enc.h"

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void global_init_ecall(Stats * stats) {
	mem_allocator_enc.init(g_part_cnt_enc, MEM_SIZE / g_part_cnt_enc);

	stats_enc = stats;

    glob_manager_enc = (ManagerEnc *) _mm_malloc(sizeof(ManagerEnc), 64);
	glob_manager_enc->init();

	if (g_cc_alg_enc == DL_DETECT) 
		dl_detector.init();

#if CC_ALG == HSTORE
	part_lock_man.init();
#elif CC_ALG == OCC
	occ_man.init();
#endif

}

void index_init_ecall(int part_cnt, table_t * table, std::string iname, uint64_t bucket_cnt) {
	IndexEnc * index = (IndexEnc *) _mm_malloc(sizeof(IndexEnc), 64);
	new(index) IndexEnc();
	index->init(part_cnt, table, bucket_cnt);
	tab_map->_tables[table->get_table_name()] = table;
	tab_map->_indexes[iname] = index;
}

void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd) {
	switch (WORKLOAD) {
		case YCSB :
			m_txn = (ycsb_txn_man *) _mm_malloc( sizeof(ycsb_txn_man), 64 );
			new(m_txn) ycsb_txn_man();
			break;
		case TPCC :
			m_txn = (tpcc_txn_man *) _mm_malloc( sizeof(tpcc_txn_man), 64 );
			new(m_txn) tpcc_txn_man();
			break;
		case TEST :
			m_txn = (TestTxnMan *) _mm_malloc( sizeof(TestTxnMan), 64 );
			new(m_txn) TestTxnMan();
			break;
		default:
			assert(false);
	}

	m_txn->init(h_thd, h_thd->_wl, h_thd->get_thd_id());

	glob_manager_enc->set_txn_man(m_txn);
}

RC run_txn_ecall(thread_t * h_thd, ts_t txn_ts) {
	// pthread_mutex_lock(&mutex);
	RC rc = RCOK;
	txn_man * m_txn;
	uint64_t thd_id = h_thd->get_thd_id();

	assert (glob_manager_enc);
	if (!glob_manager_enc->get_txn_man(thd_id)) {
		init_txn_in_enc(m_txn, h_thd);
	} else {
		m_txn = glob_manager_enc->get_txn_man(thd_id);
	}
	assert (m_txn);

	generate_txn_ocall(h_thd, h_thd->m_query);

	base_query * m_query = h_thd->m_query;
	assert (m_query);

	// generate_txn_for_run(m_query);
	// m_txn->abort_cnt = 0;


	m_txn->set_txn_id(thd_id + glob_manager_enc->get_thd_txn_id(thd_id) * g_thread_cnt_enc);
	glob_manager_enc->set_thd_txn_id(thd_id);

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
#elif CC_ALG == MVCC || CC_ALG == HEKATON
	glob_manager_enc->add_ts(get_thd_id(), m_txn->get_ts());
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
	// pthread_mutex_unlock(&mutex);
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
