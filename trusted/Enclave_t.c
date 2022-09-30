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

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[3];
} g_ecall_table = {
	3,
	{
		{(void*)(uintptr_t)sgx_ec_global_init, 0, 0},
		{(void*)(uintptr_t)sgx_ec_index_init, 0, 0},
		{(void*)(uintptr_t)sgx_ec_run_txn, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[3][3];
} g_dyn_entry_table = {
	3,
	{
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
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

