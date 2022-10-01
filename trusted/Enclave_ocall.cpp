#include "common/api.h"

#include "Enclave_t.h"

#include "sgx_error.h"

#include "common/config.h"

#ifdef USE_SGX

void generate_txn_ocall(thread_t * h_thd, base_query *& m_query) {
  void* ret = nullptr;
  auto status = oc_gen_txn(&ret, (void*) h_thd);
  if (status != SGX_SUCCESS) {
    oc_debug_print("txn ocall failed\n");
  }
  m_query = (base_query*) ret;
}

ts_t get_cur_time_ocall() {
  uint64_t time;
  sgx_status_t status = oc_get_current_time(&time);
  if (status != SGX_SUCCESS) {
    oc_debug_print("get cur time ocall failed\n");
  }
  return time;
}

// here used strlen. ensure the untrusted side cstring terminate with '\0'
std::string get_bucket_ocall(void * index, int part_id, int bkt_idx) {
  char* ptr = nullptr;
  sgx_status_t status = oc_get_bucket_ocall(&ptr, index, part_id, bkt_idx);
  if (status != SGX_SUCCESS) {
    oc_debug_print("get bucket ocall failed\n");
  }
  return std::string{ptr, strlen(ptr)};
}

#endif // USE_SGX
