#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


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
	const char* ms_iname;
	size_t ms_len;
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

static sgx_status_t SGX_CDECL sgx_ec_global_init(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ec_global_init_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ec_global_init_t* ms = SGX_CAST(ms_ec_global_init_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_stats = ms->ms_stats;



	ec_global_init(_tmp_stats);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ec_index_init(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ec_index_init_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ec_index_init_t* ms = SGX_CAST(ms_ec_index_init_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_table = ms->ms_table;
	const char* _tmp_iname = ms->ms_iname;
	size_t _tmp_len = ms->ms_len;
	size_t _len_iname = _tmp_len;
	char* _in_iname = NULL;

	CHECK_UNIQUE_POINTER(_tmp_iname, _len_iname);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_iname != NULL && _len_iname != 0) {
		if ( _len_iname % sizeof(*_tmp_iname) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_iname = (char*)malloc(_len_iname);
		if (_in_iname == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_iname, _len_iname, _tmp_iname, _len_iname)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ec_index_init(ms->ms_part_cnt, _tmp_table, (const char*)_in_iname, _tmp_len, ms->ms_bucket_cnt);

err:
	if (_in_iname) free(_in_iname);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ec_run_txn(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ec_run_txn_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ec_run_txn_t* ms = SGX_CAST(ms_ec_run_txn_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_h_thd = ms->ms_h_thd;



	ms->ms_retval = ec_run_txn(_tmp_h_thd, ms->ms_start_time);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ec_update_hash_value(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ec_update_hash_value_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ec_update_hash_value_t* ms = SGX_CAST(ms_ec_update_hash_value_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_iname = ms->ms_iname;
	size_t _tmp_len = ms->ms_len;
	size_t _len_iname = _tmp_len;
	char* _in_iname = NULL;

	CHECK_UNIQUE_POINTER(_tmp_iname, _len_iname);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_iname != NULL && _len_iname != 0) {
		if ( _len_iname % sizeof(*_tmp_iname) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_iname = (char*)malloc(_len_iname);
		if (_in_iname == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_iname, _len_iname, _tmp_iname, _len_iname)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ec_update_hash_value(ms->ms_part_id, ms->ms_bkt_idx, ms->ms_hash, (const char*)_in_iname, _tmp_len);

err:
	if (_in_iname) free(_in_iname);
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[4];
} g_ecall_table = {
	4,
	{
		{(void*)(uintptr_t)sgx_ec_global_init, 0, 0},
		{(void*)(uintptr_t)sgx_ec_index_init, 0, 0},
		{(void*)(uintptr_t)sgx_ec_run_txn, 0, 0},
		{(void*)(uintptr_t)sgx_ec_update_hash_value, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[11][4];
} g_dyn_entry_table = {
	11,
	{
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
		{0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL oc_gen_txn(void** retval, void* h_thd)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_oc_gen_txn_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_gen_txn_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_gen_txn_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_gen_txn_t));
	ocalloc_size -= sizeof(ms_oc_gen_txn_t);

	ms->ms_h_thd = h_thd;
	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL oc_get_current_time(uint64_t* retval)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_oc_get_current_time_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_get_current_time_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_get_current_time_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_get_current_time_t));
	ocalloc_size -= sizeof(ms_oc_get_current_time_t);

	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL oc_get_bucket_ocall(char** retval, void* index, int part_id, int bkt_idx)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_oc_get_bucket_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_get_bucket_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_get_bucket_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_get_bucket_ocall_t));
	ocalloc_size -= sizeof(ms_oc_get_bucket_ocall_t);

	ms->ms_index = index;
	ms->ms_part_id = part_id;
	ms->ms_bkt_idx = bkt_idx;
	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL oc_debug_print(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_oc_debug_print_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_debug_print_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_debug_print_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_debug_print_t));
	ocalloc_size -= sizeof(ms_oc_debug_print_t);

	if (str != NULL) {
		ms->ms_str = (const char*)__tmp;
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}
	
	status = sgx_ocall(3, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL oc_async_hash_value(const char* iname, size_t len, int part_id, uint64_t bkt_idx, uint64_t hash)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_iname = len;

	ms_oc_async_hash_value_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_async_hash_value_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(iname, _len_iname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iname != NULL) ? _len_iname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_async_hash_value_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_async_hash_value_t));
	ocalloc_size -= sizeof(ms_oc_async_hash_value_t);

	if (iname != NULL) {
		ms->ms_iname = (const char*)__tmp;
		if (_len_iname % sizeof(*iname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, iname, _len_iname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iname);
		ocalloc_size -= _len_iname;
	} else {
		ms->ms_iname = NULL;
	}
	
	ms->ms_len = len;
	ms->ms_part_id = part_id;
	ms->ms_bkt_idx = bkt_idx;
	ms->ms_hash = hash;
	status = sgx_ocall(4, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL oc_send_logs(const char* iname, size_t len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_iname = len;

	ms_oc_send_logs_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_oc_send_logs_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(iname, _len_iname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iname != NULL) ? _len_iname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_oc_send_logs_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_oc_send_logs_t));
	ocalloc_size -= sizeof(ms_oc_send_logs_t);

	if (iname != NULL) {
		ms->ms_iname = (const char*)__tmp;
		if (_len_iname % sizeof(*iname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, iname, _len_iname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iname);
		ocalloc_size -= _len_iname;
	} else {
		ms->ms_iname = NULL;
	}
	
	ms->ms_len = len;
	status = sgx_ocall(5, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(int);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	void *__tmp_cpuinfo = NULL;

	CHECK_ENCLAVE_POINTER(cpuinfo, _len_cpuinfo);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (cpuinfo != NULL) ? _len_cpuinfo : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));
	ocalloc_size -= sizeof(ms_sgx_oc_cpuidex_t);

	if (cpuinfo != NULL) {
		ms->ms_cpuinfo = (int*)__tmp;
		__tmp_cpuinfo = __tmp;
		if (_len_cpuinfo % sizeof(*cpuinfo) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset(__tmp_cpuinfo, 0, _len_cpuinfo);
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		ocalloc_size -= _len_cpuinfo;
	} else {
		ms->ms_cpuinfo = NULL;
	}
	
	ms->ms_leaf = leaf;
	ms->ms_subleaf = subleaf;
	status = sgx_ocall(6, ms);

	if (status == SGX_SUCCESS) {
		if (cpuinfo) {
			if (memcpy_s((void*)cpuinfo, _len_cpuinfo, __tmp_cpuinfo, _len_cpuinfo)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);

	ms->ms_self = self;
	status = sgx_ocall(7, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);

	ms->ms_waiter = waiter;
	status = sgx_ocall(8, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);

	ms->ms_waiter = waiter;
	ms->ms_self = self;
	status = sgx_ocall(9, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(void*);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(waiters, _len_waiters);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (waiters != NULL) ? _len_waiters : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);

	if (waiters != NULL) {
		ms->ms_waiters = (const void**)__tmp;
		if (_len_waiters % sizeof(*waiters) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, waiters, _len_waiters)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		ocalloc_size -= _len_waiters;
	} else {
		ms->ms_waiters = NULL;
	}
	
	ms->ms_total = total;
	status = sgx_ocall(10, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

