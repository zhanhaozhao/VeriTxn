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

std::string get_bucket_ocall(std::string iname, int part_id, int bkt_idx) {
    BucketHeader* cur = ((IndexHash*)global_table_map->_indexes[iname])->load_bucket(part_id, bkt_idx);
	return cur->encode();
}

void put_bucket_ocall(void * index, int part_id, int bkt_idx, const std::string &value) {
	
}