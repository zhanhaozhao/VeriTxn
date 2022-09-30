#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */

#include "stdint.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void ec_global_init(void* stats);
void ec_index_init(int part_cnt, void* table, const char* iname, size_t len, uint64_t bucket_cnt);
int ec_run_txn(void* h_thd, uint64_t start_time);

sgx_status_t SGX_CDECL oc_gen_txn(void** retval, void* h_thd);
sgx_status_t SGX_CDECL oc_get_current_time(uint64_t* retval);
sgx_status_t SGX_CDECL oc_get_bucket_ocall(char** retval, void* index, int part_id, int bkt_idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
