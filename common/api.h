#ifndef API_H_
#define API_H_

#include "db_thread.h"

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query);

ts_t get_cur_time_ocall();

std::string get_bucket_ocall(void * index, int part_id, int bkt_idx);

void put_bucket_disk(void * index, int part_id, int bkt_idx, const std::string &value);

#endif