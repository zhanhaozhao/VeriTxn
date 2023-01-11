#include "Enclave_t.h"
// #include <string>
#include "common/global_common.h"
#include "common/thread_enc.h"
#include "system/index_enc.h"
// #include "common/stats.h"
// #include "common/table.h"
#include "system/global_enc_struct.h"

void ec_global_init(void * stats) {
  global_init_ecall(stats);
}

void ec_index_init(int part_cnt, void* table,
  const char* iname, size_t len, void* index_ptr, uint64_t bucket_cnt) {
  
  index_init_ecall(part_cnt, table, std::string{iname, len}, index_ptr,
    bucket_cnt);
}

void ec_index_load(int part_cnt, void* table,
  const char* iname, size_t len, void* index_ptr, uint64_t bucket_cnt) {
  
  index_load_ecall(part_cnt, table, std::string{iname, len}, index_ptr,
    bucket_cnt);
}

int ec_run_txn(void* h_thd, uint64_t start_time) {
  return run_txn_ecall(h_thd, start_time);
}

void ec_update_hash_value(int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts, const char* iname, size_t len) {
    IndexEnc * index_enc = (IndexEnc *) tab_map->_indexes[std::string{iname, len}];
    index_enc->update_verify_hash(part_id, bkt_idx, hash, ts);
}
