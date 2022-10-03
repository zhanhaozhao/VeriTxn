#include "api.h"
#include "global_common.h"
#include "common/helper.h"
#include "index_btree.h"
#include "index_hash.h"
#include "global_struct.h"

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query) {
	h_thd->generate_txn_for_run(m_query);
}

ts_t get_cur_time_ocall() {
	return get_sys_clock();
}

std::string get_bucket_ocall(void * index, int part_id, int bkt_idx) {
	auto cur = ((INDEX *) global_table_map->_indexes["MAIN_INDEX"])->load_bucket(part_id, bkt_idx);
    // BucketHeader* cur = ((INDEX *) index)->load_bucket(part_id, bkt_idx);
	return cur->encode();
}

void put_bucket_ocall(void * index, int part_id, int bkt_idx, const std::string &value) {
	
}