#ifndef GLOBAL_ENC_H_
#define GLOBAL_ENC_H_

// #include "stdint.h"
// #include <unistd.h>
// #include <cstddef>
// #include <cstdlib>
// #include <cassert>
// #include <stdio.h>
// #include <iostream>
// #include <fstream>
// #include <string.h>
// #include <typeinfo>
// #include <list>
// #include <mm_malloc.h>
// #include <map>
// #include <set>
// #include <string>
// #include <vector>
// #include <sstream>
// #include <time.h> 
// #include <sys/time.h>
// #include <math.h>

// #include "pthread.h"
// #include "config.h"
#include "common/global_common.h"

// #include "common/helper.h"
// #include "common/stats.h"
// #include "common/mem_alloc.h"

// #include "dl_detect.h"
// #include "manager_enc.h"
// #include "plock.h"
// #include "occ.h"
// #ifndef NOGRAPHITE
// #include "carbon_user.h"
// #endif

// using namespace std;

// class mem_alloc;
// class Stats;
// class DL_detect;
// class Manager;
// class Query_queue;
// class Plock;
// class OptCC;
// class VLLMan;

// typedef uint32_t UInt32;
// typedef int32_t SInt32;
// typedef uint64_t UInt64;
// typedef int64_t SInt64;

// typedef uint64_t ts_t; // time stamp type

// /******************************************/
// // Global Data Structure 
// /******************************************/
// extern mem_alloc mem_allocator_enc;
// extern Stats * stats_enc;
// extern DL_detect dl_detector;

// extern ManagerEnc * glob_manager_enc;

// extern Query_queue * query_queue;
// extern Plock part_lock_man;
// extern OptCC occ_man;


// extern bool volatile warmup_finish;
// extern bool volatile enable_thread_mem_pool;
// extern pthread_barrier_t warmup_bar;
// #ifndef NOGRAPHITE
// extern carbon_barrier_t enable_barrier;
// #endif

//typedef std::pair<void*, size_t> DFlow;
#define DFlow std::string
//typedef std::string DFlow;
class Logger_generate;
extern Logger_generate log_generate;
// /******************************************/
// // Global Parameter
// /******************************************/
extern bool g_part_alloc_enc;
extern bool g_mem_pad_enc;
// extern bool g_prt_lat_distr;
extern UInt32 g_part_cnt_enc;
// extern UInt32 g_virtual_part_cnt;
extern UInt32 g_thread_cnt_enc;
// extern ts_t g_abort_penalty; 
extern bool g_central_man_enc;
extern UInt32 g_ts_alloc_enc;
// extern bool g_key_order;
extern bool g_no_dl_enc;
// extern ts_t g_timeout;
// extern ts_t g_dl_loop_detect;
extern bool g_ts_batch_alloc_enc;
extern UInt32 g_ts_batch_num_enc;
extern uint32_t g_max_log_entry_size_enc;
ts_t get_enc_time();

// extern map<string, string> g_params;

// // YCSB
extern UInt32 g_cc_alg_enc;

#if INDEX_STRUCT  == IDX_HASH
    #define INDEX_ENC IndexEnc
    #define PAGE_ENC BucketHeader_ENC
#else
#define INDEX_ENC IndexBTEnc
#define PAGE_ENC  BTNode
#endif

// extern ts_t g_query_intvl;
// extern UInt32 g_part_per_txn;
// extern double g_perc_multi_part;
// extern double g_read_perc;
// extern double g_write_perc;
// extern double g_zipf_theta;
// extern UInt64 g_synth_table_size;
// extern UInt32 g_req_per_query;
// extern UInt32 g_field_per_tuple;
// extern UInt32 g_init_parallelism;

// // TPCC
// extern UInt32 g_num_wh;
// extern double g_perc_payment;
extern bool g_wh_update_enc;
// extern char * output_file;
// extern UInt32 g_max_items;
// extern UInt32 g_cust_per_dist;

// enum RC { RCOK, Commit, Abort, WAIT, ERROR, FINISH};

// /* Thread */
// typedef uint64_t txnid_t;

// /* Txn */
// typedef uint64_t txn_t;

// /* Table and Row */
// typedef uint64_t rid_t; // row id
// typedef uint64_t pgid_t; // page id



// /* INDEX */
// enum latch_t {LATCH_EX, LATCH_SH, LATCH_NONE};
// // accessing type determines the latch type on nodes
// enum idx_acc_t {INDEX_INSERT, INDEX_READ, INDEX_NONE};
// typedef uint64_t idx_key_t; // key id for index
// typedef uint64_t (*func_ptr)(idx_key_t);	// part_id func_ptr(index_key);

// /* general concurrency control */
// enum access_t {RD, WR, XP, SCAN};
// /* LOCK */
// enum lock_t {LOCK_EX, LOCK_SH, LOCK_NONE };
// /* TIMESTAMP */
// enum TsType {R_REQ, W_REQ, P_REQ, XP_REQ}; 

// // principal index structure. The workload may decide to use a different 
// // index structure for specific purposes. (e.g. non-primary key access should use hash)
// #if (INDEX_STRUCT == IDX_BTREE)
// #define INDEX		index_btree
// #else  // IDX_HASH
// #define INDEX		IndexHash
// #endif

// /************************************************/
// // constants
// /************************************************/
// #ifndef UINT64_MAX
// #define UINT64_MAX 		18446744073709551615UL
// #endif // UINT64_MAX

#endif