#include "Enclave_t.h"

#include "common/thread_enc.h"

#include <string>

#include "common/global_common.h"
#include "common/stats.h"
#include "common/table.h"

void ec_global_init(Stats * stats) {
  global_init_ecall(stats);
}

void ec_index_init(int part_cnt, void* table,
  const char* iname, size_t len, uint64_t bucket_cnt) {
  
  index_init_ecall(part_cnt, (table_t*) table, std::string{iname, len},
    bucket_cnt);
}

RC ec_run_txn(void* h_thd, uint64_t start_time) {
  return run_txn_ecall((thread_t*) h_thd, (ts_t) start_time);
}
