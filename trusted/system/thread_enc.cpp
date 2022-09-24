#include "thread_enc.h"
#include "ycsb_txn.h"
#include "global_enc.h"
#include "api.h"
#include "manager.h"
#include "occ.h"

void global_init() {

    glob_manager = (Manager *) _mm_malloc(sizeof(Manager), 64);
	glob_manager->init();

	main_ocall();

	if (g_cc_alg == DL_DETECT) 
		dl_detector.init();

}

void global_init2() {

#if CC_ALG == HSTORE
part_lock_man.init();
#elif CC_ALG == OCC
occ_man->init();
#elif CC_ALG == VLL
vll_man.init();
#endif
    
}

void run_txn_in_enc(thread_t * h_thd) {

	txn_man * m_txn;
	m_txn = (ycsb_txn_man *)
		_mm_malloc( sizeof(ycsb_txn_man), 64 );
	new(m_txn) ycsb_txn_man();
	m_txn->init(h_thd, h_thd->_wl, h_thd->get_thd_id());

	// assert (rc == RCOK);
	glob_manager->set_txn_man(m_txn);
}