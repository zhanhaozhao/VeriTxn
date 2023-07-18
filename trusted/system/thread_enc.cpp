#include "thread_enc.h"

#include "ycsb_txn.h"
#include "tpcc_txn.h"
#include "test_txn.h"
#include "global_enc_struct.h"
#include "api.h"
#include "logger_enc.h"
// #include "manager.h"
#include "occ.h"
#include "index_enc.h"
#include "mem_helper_enc.h"
#include "global_enc.h"
#include "index_btree_enc.h"

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void global_init_ecall(void * stats) {
	// mem_allocator_enc.init(g_part_cnt_enc, MEM_SIZE / g_part_cnt_enc);

	stats_enc = (Stats *) stats;

    // glob_manager_enc = (ManagerEnc *) aligned_alloc(64, sizeof(ManagerEnc));
	glob_manager_enc = (ManagerEnc *) malloc(sizeof(ManagerEnc));
	glob_manager_enc->init();

	if (g_cc_alg_enc == DL_DETECT) 
		dl_detector.init();

#if CC_ALG == HSTORE
	part_lock_man.init();
#elif CC_ALG == OCC
	occ_man.init();
#endif
	// printf("Initializing trusted log generator... ");
	// fflush(stdout);
	log_generate.init();
	// printf("Done\n");

}

void index_init_ecall(int part_cnt, void * table, std::string iname, void * index_ptr, uint64_t bucket_cnt) {
    INDEX_ENC * index = new INDEX_ENC();
    assert(table);
	table_t * tbl = (table_t *) table;

#if INDEX_STRUCT == IDX_HASH
	index->init(part_cnt, tbl, bucket_cnt);
#else
	index->init(part_cnt);
	index->table = tbl;
    assert(tbl);
#endif
    std::string tname = std::string(tbl->get_table_name());
    assert(tname.length() > 0);
//    printf("init table %s\n", tname.c_str());
	tab_map->_tables[tname] = table;
	tab_map->_indexes[iname] = index;
	inner_index_map->_indexes[iname] = index_ptr;
	index->index_name = iname;
}

void index_load_ecall(int part_cnt, void * table, std::string iname, void * index_ptr, uint64_t bucket_cnt) {
    INDEX_ENC * index = (INDEX_ENC *)tab_map->_indexes[iname];
#if INDEX_STRUCT == IDX_HASH
    #if PRE_LOAD == 1
    for (int i=0;i<part_cnt;i++)
        for (uint64_t j=0;j<bucket_cnt;j++) {
            index->load_bucket(iname, i, j);
            if (!index->_cache->can_force_load())
                break;
        }
    index->init_ts(part_cnt);
    index->preloading = false;
    #endif
#else
    index->load_all(iname);
#endif
}


void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd) {
	switch (WORKLOAD) {
		case YCSB :
			// m_txn = (ycsb_txn_man *) aligned_alloc(64, sizeof(ycsb_txn_man));
			m_txn = (ycsb_txn_man *) malloc(sizeof(ycsb_txn_man));
			new(m_txn) ycsb_txn_man();
			break;
		case TPCC :
			// m_txn = (tpcc_txn_man *) aligned_alloc(64, sizeof(tpcc_txn_man));
			m_txn = (tpcc_txn_man *) malloc(sizeof(tpcc_txn_man));
			new(m_txn) tpcc_txn_man();
            assert(m_txn->row_cnt == 0);
			break;
		case TEST :
			// m_txn = (TestTxnMan *) aligned_alloc(64, sizeof(TestTxnMan));
			m_txn = (TestTxnMan *) malloc(sizeof(TestTxnMan));
			new(m_txn) TestTxnMan();
			break;
		default:
			assert(false);
	}

	m_txn->init(h_thd, h_thd->_wl, h_thd->get_thd_id());

	glob_manager_enc->set_txn_man(m_txn);
}

//#include "Enclave_t.h"

int run_txn_ecall(void * thd, ts_t txn_ts) {
	// pthread_mutex_lock(&mutex);
//	if (txn_ts % 100 == 0) {
////        oc_debug_print("running transaction!!!");
//        oc_debug_print(std::to_string(txn_ts).c_str());
//	}

	thread_t * h_thd = (thread_t *) thd;

	RC rc = RCOK;
	txn_man * m_txn = nullptr;
	uint64_t thd_id = h_thd->get_thd_id();

	assert (glob_manager_enc);
	if (!glob_manager_enc->get_txn_man(thd_id)) {
		init_txn_in_enc(m_txn, h_thd);
	} else {
		m_txn = glob_manager_enc->get_txn_man(thd_id);
	}
	assert (m_txn);

//	generate_txn_ocall(h_thd, h_thd->m_query);

	base_query * m_query = h_thd->m_query;
	assert (m_query);

	// generate_txn_for_run(m_query);
	// m_txn->abort_cnt = 0;


	m_txn->set_txn_id(thd_id + glob_manager_enc->get_thd_txn_id(thd_id) * g_thread_cnt_enc);
	glob_manager_enc->set_thd_txn_id(thd_id);

    m_txn->set_ts(txn_ts);  // always needed for VeriTXN.
    m_txn->begin_t = get_enc_time();

//	if ((CC_ALG == HSTORE && !HSTORE_LOCAL_TS)
//			|| CC_ALG == MVCC
//			|| CC_ALG == HEKATON
//			|| CC_ALG == TIMESTAMP)
//		m_txn->set_ts(txn_ts);

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
			rc = (RC) runTest(m_txn);
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


void update_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts) {
	IndexEnc * index_enc = (IndexEnc *) tab_map->_indexes[index_name];
  	index_enc->update_verify_hash(part_id, bkt_idx, hash, ts);
}


int runTest(void * txn)
{
	txn_man * txn_t = (txn_man *) txn;
	RC rc = RCOK;
	if (g_test_case == READ_WRITE) {
// 		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
// #if CC_ALG == OCC
// 		txn->start_ts = get_next_ts(); 
// #endif
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 1);
		// printf("READ_WRITE TEST PASSED\n");
		return FINISH;
	}
	else if (g_test_case == CONFLICT) {
		rc = ((TestTxnMan *)txn_t)->run_txn(g_test_case, 0);
		if (rc == RCOK)
			return FINISH;
		else 
			return rc;
	}
	assert(false);
	return RCOK;
}
