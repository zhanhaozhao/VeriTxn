#include "common/api.h"

#include "Enclave_t.h"

#include "sgx_error.h"

#include "common/config.h"

#if USE_SGX == 1

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query) {
  void* ret = nullptr;
  auto status = oc_gen_txn(&ret, (void*) h_thd);
  if (status != SGX_SUCCESS) {
    oc_debug_print("txn ocall failed\n");
  }
  m_query = (base_query*) ret;
}

//ts_t get_cur_time_ocall() {
//  uint64_t time;
//  sgx_status_t status = oc_get_current_time(&time);
//  if (status != SGX_SUCCESS) {
//    oc_debug_print("get cur time ocall failed\n");
//  }
//  return time;
//}

// here used strlen. ensure the untrusted side cstring terminate with '\0'
std::string get_bucket_ocall(std::string iname, int part_id, int bkt_idx) {
  char* ptr = nullptr;
  sgx_status_t status = oc_get_bucket_ocall(&ptr, iname.c_str(), iname.size(), part_id, bkt_idx);
  if (status != SGX_SUCCESS) {
    oc_debug_print("get bucket ocall failed\n");
  }
  return std::string{ptr, strlen(ptr)};
}

void async_hash_value(std::string index_name, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts) {
  oc_async_hash_value(index_name.c_str(), index_name.size(), part_id, bkt_idx, hash, ts);
}

void send_logs(std::string logs, int size, uint64_t thd_id) {
  oc_send_logs(logs.c_str(), logs.size(), thd_id);
}

void sync_bucket_from_disk(std::string index_name, size_t len, int part_id, uint64_t pg_id){

  oc_sync_bucket_from_disk(index_name.c_str(), len, part_id, pg_id);
}

#endif // USE_SGX
