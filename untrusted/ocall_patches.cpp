#include "common/config.h"

#if USE_SGX == 1
#include "common/api.h"

#include <memory>
#include <mutex>
#include <list>
#include <string>


#include "db_thread.h"

#include "Enclave_u.h"

// need to handle and properly use ocall to clean this 
std::mutex bkt_l_mtx;
std::list<std::string> bkt_l;


void* oc_gen_txn(void* h_thd) {
  base_query* m_query;
  generate_txn_ocall((thread_t*) h_thd, m_query);
  return m_query;
}

//uint64_t oc_get_current_time() {
//  return get_cur_time_ocall();
//}

char* oc_get_bucket_ocall(const char * iname, size_t len, int part_id, int bkt_idx) {
  
  std::string bucket_raw = get_bucket_ocall(std::string{iname, len}, part_id, bkt_idx);
  
  char* ret;
  {
    std::lock_guard<std::mutex> lg(bkt_l_mtx);
    bkt_l.emplace_back(std::move(bucket_raw));
    ret = const_cast<char*>(bkt_l.back().c_str());
  }
  return ret;
}


void oc_debug_print(const char* str) {
  printf("ocall debug print: %s\n", str);
}

void oc_async_hash_value(const char* iname, size_t len, int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts) {
  async_hash(std::string{iname, len}, part_id, bkt_idx, hash, ts);
}

void oc_send_logs(const char* logs, size_t len, int thd_id) {
  // LogRecord** records = (LogRecord**) logs; 
  untrust_send_logs(std::string{logs, len}, thd_id);
}

void oc_sync_bucket_from_disk(const char* index_name, size_t len, int part_id, int pg_id) {

  sync_bucket_from_disk_internal(std::string{index_name, len}, len, part_id, pg_id);
}

#endif // USE_SGX
