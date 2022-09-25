#include "api.h"
#include "helper.h"

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query) {
	h_thd->generate_txn_for_run(m_query);
}

ts_t get_cur_time_ocall() {
	return get_sys_clock();
}