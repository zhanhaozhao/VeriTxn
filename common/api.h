#ifndef API_H_
#define API_H_

#include "db_thread.h"
// #include "txn.h"
#include "log.h"

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query);

ts_t get_cur_time_ocall();

std::string get_bucket_ocall(std::string iname, int part_id, int bkt_idx);

void put_bucket_disk(void * index, int part_id, int bkt_idx, const std::string &value);

void async_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash);
void update_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash);

void send_logs(std::string logs, int size);
void async_hash(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash);
void untrust_send_logs(std::string logs);
#endif