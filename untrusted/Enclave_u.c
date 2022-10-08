#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_ec_global_init_t {
	void* ms_stats;
} ms_ec_global_init_t;

typedef struct ms_ec_index_init_t {
	int ms_part_cnt;
	void* ms_table;
	const char* ms_iname;
	size_t ms_len;
	uint64_t ms_bucket_cnt;
} ms_ec_index_init_t;

typedef struct ms_ec_run_txn_t {
	int ms_retval;
	void* ms_h_thd;
	uint64_t ms_start_time;
} ms_ec_run_txn_t;

typedef struct ms_ec_update_hash_value_t {
	int ms_part_id;
	uint64_t ms_bkt_idx;
	uint64_t ms_hash;
	const char* ms_iname;
	size_t ms_len;
} ms_ec_update_hash_value_t;

typedef struct ms_oc_gen_txn_t {
	void* ms_retval;
	void* ms_h_thd;
} ms_oc_gen_txn_t;

typedef struct ms_oc_get_current_time_t {
	uint64_t ms_retval;
} ms_oc_get_current_time_t;

typedef struct ms_oc_get_bucket_ocall_t {
	char* ms_retval;
	void* ms_index;
	int ms_part_id;
	int ms_bkt_idx;
} ms_oc_get_bucket_ocall_t;

typedef struct ms_oc_debug_print_t {
	const char* ms_str;
} ms_oc_debug_print_t;

typedef struct ms_oc_async_hash_value_t {
	const char* ms_iname;
	size_t ms_len;
	int ms_part_id;
	uint64_t ms_bkt_idx;
	uint64_t ms_hash;
} ms_oc_async_hash_value_t;

typedef struct ms_oc_send_logs_t {
	void* ms_logs;
	int ms_size;
} ms_oc_send_logs_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	const void* ms_waiter;
	const void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	const void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL Enclave_oc_gen_txn(void* pms)
{
	ms_oc_gen_txn_t* ms = SGX_CAST(ms_oc_gen_txn_t*, pms);
	ms->ms_retval = oc_gen_txn(ms->ms_h_thd);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_oc_get_current_time(void* pms)
{
	ms_oc_get_current_time_t* ms = SGX_CAST(ms_oc_get_current_time_t*, pms);
	ms->ms_retval = oc_get_current_time();

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_oc_get_bucket_ocall(void* pms)
{
	ms_oc_get_bucket_ocall_t* ms = SGX_CAST(ms_oc_get_bucket_ocall_t*, pms);
	ms->ms_retval = oc_get_bucket_ocall(ms->ms_index, ms->ms_part_id, ms->ms_bkt_idx);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_oc_debug_print(void* pms)
{
	ms_oc_debug_print_t* ms = SGX_CAST(ms_oc_debug_print_t*, pms);
	oc_debug_print(ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_oc_async_hash_value(void* pms)
{
	ms_oc_async_hash_value_t* ms = SGX_CAST(ms_oc_async_hash_value_t*, pms);
	oc_async_hash_value(ms->ms_iname, ms->ms_len, ms->ms_part_id, ms->ms_bkt_idx, ms->ms_hash);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_oc_send_logs(void* pms)
{
	ms_oc_send_logs_t* ms = SGX_CAST(ms_oc_send_logs_t*, pms);
	oc_send_logs(ms->ms_logs, ms->ms_size);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall(ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall(ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall(ms->ms_waiter, ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall(ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[11];
} ocall_table_Enclave = {
	11,
	{
		(void*)Enclave_oc_gen_txn,
		(void*)Enclave_oc_get_current_time,
		(void*)Enclave_oc_get_bucket_ocall,
		(void*)Enclave_oc_debug_print,
		(void*)Enclave_oc_async_hash_value,
		(void*)Enclave_oc_send_logs,
		(void*)Enclave_sgx_oc_cpuidex,
		(void*)Enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)Enclave_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};
sgx_status_t ec_global_init(sgx_enclave_id_t eid, void* stats)
{
	sgx_status_t status;
	ms_ec_global_init_t ms;
	ms.ms_stats = stats;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ec_index_init(sgx_enclave_id_t eid, int part_cnt, void* table, const char* iname, size_t len, uint64_t bucket_cnt)
{
	sgx_status_t status;
	ms_ec_index_init_t ms;
	ms.ms_part_cnt = part_cnt;
	ms.ms_table = table;
	ms.ms_iname = iname;
	ms.ms_len = len;
	ms.ms_bucket_cnt = bucket_cnt;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ec_run_txn(sgx_enclave_id_t eid, int* retval, void* h_thd, uint64_t start_time)
{
	sgx_status_t status;
	ms_ec_run_txn_t ms;
	ms.ms_h_thd = h_thd;
	ms.ms_start_time = start_time;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ec_update_hash_value(sgx_enclave_id_t eid, int part_id, uint64_t bkt_idx, uint64_t hash, const char* iname, size_t len)
{
	sgx_status_t status;
	ms_ec_update_hash_value_t ms;
	ms.ms_part_id = part_id;
	ms.ms_bkt_idx = bkt_idx;
	ms.ms_hash = hash;
	ms.ms_iname = iname;
	ms.ms_len = len;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	return status;
}

