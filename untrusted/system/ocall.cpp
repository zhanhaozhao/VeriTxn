#include "api.h"
#include "global_common.h"
#include "global.h"
#include "msg_queue.h"
#include "logger.h"
#include "common/helper.h"
#include "message.h"
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

void async_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash) {
  async_hash(index_name, part_id, bkt_idx, hash);
}

void async_hash(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash){
	if (g_node_id != 0) return;
	for (int i = 1; i < NODE_CNT; i++) {
		assert(g_node_id == 0);
		//! here we set that the node with id 0 must be the rw operation node. 
		Message* msg = Message::create_message(ASYNC_HASH);
		AsyncHashMessage* asymsg = (AsyncHashMessage*) msg;
		asymsg->init(index_name, part_id, bkt_idx, hash);
		msg_queue.enqueue(0, asymsg, i);
	}
}

void send_logs(std::string logs, int size) {
	logger.buf_to_log(logs);
	// 	LogRecord* clog = NULL;
	// 	clog = (LogRecord*)logs[i];
	// 	logger.enqueueRecord(logs[i]);
	// }
}

void untrust_send_logs(std::string logs) {
	logger.buf_to_log(logs);
	// 	LogRecord* clog = NULL;
	// 	clog = (LogRecord*)logs[i];
	// 	logger.enqueueRecord(logs[i]);
	// }
}