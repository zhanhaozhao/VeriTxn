#include "common/config.h"

#ifdef USE_SGX
#include "common/thread_enc.h"

#include "Enclave_u.h"

#include "sgx_error.h"

#include "untrusted/system/global.h"


extern sgx_enclave_id_t enclave_id;
void global_init_ecall(void * stats) {
  sgx_status_t status = ec_global_init(enclave_id, stats);
  if (status != SGX_SUCCESS) {
    printf("global init failed : error %d - %#x.\n", status,
      status);
  }
}

void index_init_ecall(int part_cnt, void * table,
  std::string iname, void * index_ptr, uint64_t bucket_cnt) {
  sgx_status_t status = ec_index_init(enclave_id, part_cnt, table, iname.c_str(),
    iname.size(), index_ptr, bucket_cnt);
  if (status != SGX_SUCCESS) {
    printf("index init failed : error %d - %#x.\n", status,
      status);
  }
}

int run_txn_ecall(void * h_thd, uint64_t start_time) {
  int ret;
  assert(h_thd);
  sgx_status_t status = ec_run_txn(enclave_id, &ret, h_thd, start_time);
  if (status != SGX_SUCCESS) {
    printf("run txn failed : error %d - %#x.\n", status,
      status);
    return -1;
  }
  return ret;
}

#endif // USE_SGX
