#ifndef API_H_
#define API_H_

#include "db_thread.h"
// #include "txn.h"
#include "log.h"

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query);

//ts_t get_cur_time_ocall();

std::string get_bucket_ocall(std::string iname, int part_id, int bkt_idx);

void put_bucket_disk(void * index, int part_id, int bkt_idx, const std::string &value);

void async_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts);
void update_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts);

void send_logs(std::string logs, int size, uint64_t thd_id);
void async_hash(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts);
void untrust_send_logs(std::string logs, uint64_t thd_id);

void sync_bucket_from_disk(std::string index_name, size_t len, int part_id, uint64_t pg_id);
void sync_bucket_from_disk_internal(std::string index_name, size_t len, int part_id, uint64_t pg_id);

#endif