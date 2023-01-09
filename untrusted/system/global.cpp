#include "global.h"
#include "transport.h"
#include "msg_queue.h"
// #include "logger.h"
#include "sim_manager.h"
// #include "plock.h"
// #include "occ.h"

#if USE_SGX == 1
#include "sgx_eid.h"
#endif // USE_SGX

// mem_alloc mem_allocator;
// Stats stats;
// // DL_detect dl_detector;

// Manager * glob_manager;
// Query_queue * query_queue;
Transport tport_man;
MessageQueue msg_queue;
SimManager * simulation;
UInt32 g_node_id = 0;
UInt32 g_node_cnt = NODE_CNT;
#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
Logqueue **log_queues;
#endif

// // Plock part_lock_man;
// // OptCC occ_man;


bool volatile warmup_finish = false;
bool volatile enable_thread_mem_pool = false;
pthread_barrier_t warmup_bar;
pthread_barrier_t log_bar;
// #ifndef NOGRAPHITE
// carbon_barrier_t enable_barrier;
// #endif

ts_t g_abort_penalty = ABORT_PENALTY;
bool g_central_man = CENTRAL_MAN;
UInt32 g_ts_alloc = TS_ALLOC;
bool g_key_order = KEY_ORDER;
bool g_no_dl = NO_DL;
ts_t g_timeout = TIMEOUTDL;
ts_t g_dl_loop_detect = DL_LOOP_DETECT;
bool g_ts_batch_alloc = TS_BATCH_ALLOC;
UInt32 g_ts_batch_num = TS_BATCH_NUM;

bool g_part_alloc = PART_ALLOC;
bool g_mem_pad = MEM_PAD;
UInt32 g_cc_alg = CC_ALG;
ts_t g_query_intvl = QUERY_INTVL;
UInt32 g_part_per_txn = PART_PER_TXN;
double g_perc_multi_part = PERC_MULTI_PART;
double g_read_perc = READ_PERC;
double g_write_perc = WRITE_PERC;
double g_zipf_theta = ZIPF_THETA;
bool g_prt_lat_distr = PRT_LAT_DISTR;
UInt32 g_part_cnt = PART_CNT;
UInt32 g_virtual_part_cnt = VIRTUAL_PART_CNT;
UInt32 g_thread_cnt = THREAD_CNT;
UInt64 g_synth_table_size = SYNTH_TABLE_SIZE;
UInt32 g_req_per_query = REQ_PER_QUERY;
UInt32 g_field_per_tuple = FIELD_PER_TUPLE;
UInt32 g_init_parallelism = INIT_PARALLELISM;

UInt32 g_num_wh = NUM_WH;
double g_perc_payment = PERC_PAYMENT;
bool g_wh_update = WH_UPDATE;
char * output_file = NULL;


uint64_t g_log_buffer_size = LOG_BUFFER_SIZE;
uint32_t g_max_log_entry_size = MAX_LOG_ENTRY_SIZE;
bool g_log_recover = LOG_RECOVER;
uint32_t g_num_logger = NUM_LOGGER;
uint64_t g_flush_blocksize = FLUSH_BLOCK_SIZE;
uint64_t g_read_blocksize = READ_BLOCK_SIZE;

std::map<std::string, std::string> g_params;

#if TPCC_SMALL
UInt32 g_max_items = 10000;
UInt32 g_cust_per_dist = 2000;
#else 
UInt32 g_max_items = 100000;
UInt32 g_cust_per_dist = 3000;
#endif

// enclave id
#if USE_SGX == 1
sgx_enclave_id_t enclave_id = 0;
#endif // USE_SGX

int MAX_LINE = 1024*1024;
int PORT = 50001;
int BACKLOG = 10;
int LISTENQ = 50062;
int MAX_CONNECT = 20;