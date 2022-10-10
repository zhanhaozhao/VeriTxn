#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */

#include "stdint.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OC_GEN_TXN_DEFINED__
#define OC_GEN_TXN_DEFINED__
void* SGX_UBRIDGE(SGX_NOCONVENTION, oc_gen_txn, (void* h_thd));
#endif
#ifndef OC_GET_CURRENT_TIME_DEFINED__
#define OC_GET_CURRENT_TIME_DEFINED__
uint64_t SGX_UBRIDGE(SGX_NOCONVENTION, oc_get_current_time, (void));
#endif
#ifndef OC_GET_BUCKET_OCALL_DEFINED__
#define OC_GET_BUCKET_OCALL_DEFINED__
char* SGX_UBRIDGE(SGX_NOCONVENTION, oc_get_bucket_ocall, (void* index, int part_id, int bkt_idx));
#endif
#ifndef OC_DEBUG_PRINT_DEFINED__
#define OC_DEBUG_PRINT_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, oc_debug_print, (const char* str));
#endif
#ifndef OC_ASYNC_HASH_VALUE_DEFINED__
#define OC_ASYNC_HASH_VALUE_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, oc_async_hash_value, (const char* iname, size_t len, int part_id, uint64_t bkt_idx, uint64_t hash));
#endif
#ifndef OC_SEND_LOGS_DEFINED__
#define OC_SEND_LOGS_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, oc_send_logs, (const char* iname, size_t len));
#endif
#ifndef SGX_OC_CPUIDEX_DEFINED__
#define SGX_OC_CPUIDEX_DEFINED__
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
#endif
#ifndef SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
#endif
#ifndef SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
#endif
#ifndef SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
#endif
#ifndef SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
#endif

sgx_status_t ec_global_init(sgx_enclave_id_t eid, void* stats);
sgx_status_t ec_index_init(sgx_enclave_id_t eid, int part_cnt, void* table, const char* iname, size_t len, uint64_t bucket_cnt);
sgx_status_t ec_run_txn(sgx_enclave_id_t eid, int* retval, void* h_thd, uint64_t start_time);
sgx_status_t ec_update_hash_value(sgx_enclave_id_t eid, int part_id, uint64_t bkt_idx, uint64_t hash, const char* iname, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
