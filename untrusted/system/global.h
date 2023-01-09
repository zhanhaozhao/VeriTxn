#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <time.h> 
#include <sys/time.h>
#include <mm_malloc.h>
#include <vector>
#include "common/helper.h"
#include "common/config.h"
#include "common/global_common.h"
// #include "logger.h"
// #include "logger.h"
// #ifndef NOGRAPHITE
// #include "carbon_user.h"
// #endif

// #include "mem_alloc.h"
// #include "stats.h"
// // #include "dl_detect.h"
// // #include "manager.h"
// #include "query.h"
// // #include "helper.h"
// #include "manager.h"

// #include "common/stats.h"
// #include "dl_detect.h"

// using namespace std;

// class Stats;
// class Manager;
class table_map;
class MessageQueue;
class Transport;
// class Logger;
class SimManager;
class Logqueue;
// class kvserver;
// class RemoteStorage;
// /******************************************/
// // Global Data Structure 
// /******************************************/
// extern mem_alloc mem_allocator;
// extern Stats stats;
// // extern DL_detect dl_detector;
// extern Manager * glob_manager;
// extern Query_queue * query_queue;
// // extern Plock part_lock_man;
// // extern OptCC occ_man;
extern Transport tport_man;
extern MessageQueue msg_queue;

extern SimManager * simulation;

#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
extern Logqueue **log_queues;
#endif

extern UInt32 g_node_id;
extern UInt32 g_node_cnt;

extern bool volatile warmup_finish;
extern bool volatile enable_thread_mem_pool;
extern pthread_barrier_t warmup_bar;
extern pthread_barrier_t log_bar;
// #ifndef NOGRAPHITE
// extern carbon_barrier_t enable_barrier;
// #endif

/******************************************/
// Global Parameter
/******************************************/
extern bool g_part_alloc;
extern bool g_mem_pad;
extern bool g_prt_lat_distr;
extern UInt32 g_part_cnt;
extern UInt32 g_virtual_part_cnt;
extern UInt32 g_thread_cnt;
extern ts_t g_abort_penalty; 
extern bool g_central_man;
extern UInt32 g_ts_alloc;
extern bool g_key_order;
extern bool g_no_dl;
extern ts_t g_timeout;
extern ts_t g_dl_loop_detect;
extern bool g_ts_batch_alloc;
extern UInt32 g_ts_batch_num;


extern uint64_t g_log_buffer_size;
extern uint32_t g_max_log_entry_size;
extern bool g_log_recover;
extern uint32_t g_num_logger;
extern uint64_t g_flush_blocksize;
extern uint64_t g_read_blocksize;

extern std::map<std::string, std::string> g_params;

// YCSB
extern UInt32 g_cc_alg;
extern ts_t g_query_intvl;
extern UInt32 g_part_per_txn;
extern double g_perc_multi_part;
extern double g_read_perc;
extern double g_write_perc;
extern double g_zipf_theta;
extern UInt64 g_synth_table_size;
extern UInt32 g_req_per_query;
extern UInt32 g_field_per_tuple;
extern UInt32 g_init_parallelism;

// TPCC
extern UInt32 g_num_wh;
extern double g_perc_payment;
extern bool g_wh_update;
extern char * output_file;
extern UInt32 g_max_items;
extern UInt32 g_cust_per_dist;

enum RemReqType {
    ASYNC_HASH = 0,
    INIT_DONE,
    NO_MSG
};

extern int MAX_LINE;
extern int PORT;
extern int BACKLOG;
extern int LISTENQ;
extern int MAX_CONNECT;

#endif