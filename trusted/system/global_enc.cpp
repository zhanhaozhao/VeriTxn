#include "global_enc.h"
#include "logger_enc.h"
// #include "common/mem_alloc.h"
// #include "common/stats.h"
// #include "dl_detect.h"
// #include "manager_enc.h"
// #include "common/query.h"
// #include "plock.h"
// #include "occ.h"

// mem_alloc mem_allocator_enc;
// Stats * stats_enc;
// DL_detect dl_detector;

// ManagerEnc * glob_manager_enc;

// Query_queue * query_queue;

// Plock part_lock_man;
// OptCC occ_man;


// bool volatile warmup_finish = false;
// bool volatile enable_thread_mem_pool = false;
// pthread_barrier_t warmup_bar;
// #ifndef NOGRAPHITE
// carbon_barrier_t enable_barrier;
// #endif
Logger_generate log_generate;
// ts_t g_abort_penalty = ABORT_PENALTY;
bool g_central_man_enc = CENTRAL_MAN;
UInt32 g_ts_alloc_enc = TS_ALLOC;
// bool g_key_order = KEY_ORDER;
bool g_no_dl_enc = NO_DL;
// ts_t g_timeout = TIMEOUTDL;
// ts_t g_dl_loop_detect = DL_LOOP_DETECT;
bool g_ts_batch_alloc_enc = TS_BATCH_ALLOC;
UInt32 g_ts_batch_num_enc = TS_BATCH_NUM;

bool g_part_alloc_enc = PART_ALLOC;
bool g_mem_pad_enc = MEM_PAD;
UInt32 g_cc_alg_enc = CC_ALG;
// ts_t g_query_intvl = QUERY_INTVL;
// UInt32 g_part_per_txn = PART_PER_TXN;
// double g_perc_multi_part = PERC_MULTI_PART;
// double g_read_perc = READ_PERC;
// double g_write_perc = WRITE_PERC;
// double g_zipf_theta = ZIPF_THETA;
// bool g_prt_lat_distr = PRT_LAT_DISTR;
UInt32 g_part_cnt_enc = PART_CNT;
// UInt32 g_virtual_part_cnt = VIRTUAL_PART_CNT;
UInt32 g_thread_cnt_enc = THREAD_CNT;
// UInt64 g_synth_table_size = SYNTH_TABLE_SIZE;
// UInt32 g_req_per_query = REQ_PER_QUERY;
// UInt32 g_field_per_tuple = FIELD_PER_TUPLE;
// UInt32 g_init_parallelism = INIT_PARALLELISM;

// UInt32 g_num_wh = NUM_WH;
// double g_perc_payment = PERC_PAYMENT;
bool g_wh_update_enc = WH_UPDATE;
uint32_t g_max_log_entry_size_enc = MAX_LOG_ENTRY_SIZE;
// char * output_file = NULL;

ts_t get_enc_time() {
    return get_sys_clock();
}

// map<string, string> g_params;

// #if TPCC_SMALL
// UInt32 g_max_items = 10000;
// UInt32 g_cust_per_dist = 2000;
// #else 
// UInt32 g_max_items = 100000;
// UInt32 g_cust_per_dist = 3000;
// #endif
