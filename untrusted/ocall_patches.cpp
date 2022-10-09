#include "common/config.h"

#ifdef USE_SGX
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

uint64_t oc_get_current_time() {
  return get_cur_time_ocall();
}

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

#endif // USE_SGX
