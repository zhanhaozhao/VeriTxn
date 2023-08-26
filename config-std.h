#ifndef CONFIG_H_
#define CONFIG_H_

/***********************************************/
// Simulation + Hardware
/***********************************************/
#define NODE_CNT 1
#define THREAD_CNT 4
#define PART_CNT 1
#define INPUT_CNT					1
#define OUTPUT_CNT					1
// each transaction only accesses 1 virtual partition. But the lock/ts manager and index are not aware of such partitioning. VIRTUAL_PART_CNT describes the request distribution and is only used to generate queries. For HSTORE, VIRTUAL_PART_CNT should be the same as PART_CNT.
#define VIRTUAL_PART_CNT			1
#define PAGE_SIZE					4096
#define CL_SIZE						64
// CPU_FREQ is used to get accurate timing info
#define CPU_FREQ 					2 	// in GHz/s


// # of transactions to run for warmup
#define WARMUP						0
// YCSB or TPCC
#define WORKLOAD YCSB
// print the transaction latency distribution
#define PRT_LAT_DISTR				false
#define STATS_ENABLE				true
#define TIME_ENABLE					true

#define MEM_ALLIGN					8

// [THREAD_ALLOC]
#define THREAD_ALLOC				false
#define THREAD_ARENA_SIZE			(1UL << 22)
#define MEM_PAD 					true

// [PART_ALLOC]
#define PART_ALLOC 					false
#define MEM_SIZE					(1UL << 30)
#define NO_FREE						false

/***********************************************/
// Concurrency Control
/***********************************************/
// WAIT_DIE, NO_WAIT, DL_DETECT, TIMESTAMP, MVCC, HEKATON, HSTORE, OCC, VLL, TICTOC, SILO
// TODO TIMESTAMP does not work at this moment
#define CC_ALG OCC
#define ISOLATION_LEVEL SERIALIZABLE

// all transactions acquire tuples according to the primary key order.
#define KEY_ORDER					false
// transaction roll back changes after abort
#define ROLL_BACK					true
// per-row lock/ts management or central lock/ts management
#define CENTRAL_MAN					false
#define BUCKET_CNT					31
#define ABORT_PENALTY 10 * 1000000UL   // in ns.
#define ABORT_BUFFER_SIZE			10
#define ABORT_BUFFER_ENABLE			true
// [ INDEX ]
#define ENABLE_LATCH				true
#define CENTRAL_INDEX				false
#define CENTRAL_MANAGER 			false
#define INDEX_STRUCT IDX_HASH
#define BTREE_ORDER 				16
#define INDEX_NAME_LENGTH       	16

#if WORKLOAD == YCSB
#define BTREE_NODE_NUM              (SYNTH_TABLE_SIZE  + SYNTH_TABLE_SIZE/2+10)
#else
#define BTREE_NODE_NUM              2000000
#endif

// [DL_DETECT]
#define DL_LOOP_DETECT				1000 	// 100 us
#define DL_LOOP_TRIAL				100	// 1 us
#define NO_DL						KEY_ORDER
#define TIMEOUTDL					1000000 // 1ms
// [TIMESTAMP]
#define TS_TWR						false
#define TS_ALLOC					TS_CAS
#define TS_BATCH_ALLOC				false
#define TS_BATCH_NUM				1
// [MVCC]
// when read/write history is longer than HIS_RECYCLE_LEN
// the history should be recycled.
//#define HIS_RECYCLE_LEN				10
//#define MAX_PRE_REQ					1024
//#define MAX_READ_REQ				1024
#define MIN_TS_INTVL				5000000 //5 ms. In nanoseconds
// [OCC]
#define MAX_WRITE_SET				10
#define PER_ROW_VALID				true
// [TICTOC]
#define WRITE_COPY_FORM				"data" // ptr or data
#define TICTOC_MV					false
#define WR_VALIDATION_SEPARATE		true
#define WRITE_PERMISSION_LOCK		false
#define ATOMIC_TIMESTAMP			"false"
// [TICTOC, SILO]
#define VALIDATION_LOCK				"no-wait" // no-wait or waiting
#define PRE_ABORT					"true"
#define ATOMIC_WORD					true
// [HSTORE]
// when set to true, hstore will not access the global timestamp.
// This is fine for single partition transactions.
#define HSTORE_LOCAL_TS				false
// [VLL]
#define TXN_QUEUE_SIZE_LIMIT		THREAD_CNT

// Logging type
#define LOG_DATA					1
#define LOG_COMMAND					2

/***********************************************/
// Logging
/***********************************************/
// #define LOG_COMMAND					false
// #define LOG_REDO					false
#define LOG_BATCH_TIME				10 // in ms
#define LOG_TYPE                    LOG_DATA
#define LOG_BUFFER_SIZE				(1048576 * 200)	// in bytes, 200MB
#define MAX_LOG_ENTRY_SIZE			16384 // in Bytes
#define LOG_RECOVER                 false
#define NUM_LOGGER					1 // the number of loggers
#define FLUSH_BLOCK_SIZE		1048576 // twice as best among 4096 40960 409600 4096000
#define READ_BLOCK_SIZE 419430400


/***********************************************/
// Benchmark
/***********************************************/
// max number of rows touched per transaction
#define MAX_ROW_PER_TXN				128
#define QUERY_INTVL 				1UL
#define MAX_TXN_PER_PART 10000
#define FIRST_PART_LOCAL 			true
#define MAX_TUPLE_SIZE				1024 // in bytes
// ==== [YCSB] ====
#define INIT_PARALLELISM 8
#define SYNTH_TABLE_SIZE 4194304
#define ZIPF_THETA 0.5
#define READ_PERC 0.5
#define WRITE_PERC 0.5
#define SCAN_PERC 					0
#define SCAN_LEN					20
#define PART_PER_TXN 2
#define PERC_MULTI_PART				1
#define REQ_PER_QUERY 64
#define FIELD_PER_TUPLE				10
// ==== [TPCC] ====
// For large warehouse count, the tables do not fit in memory
// small tpcc schemas shrink the table size.
#define TPCC_SMALL					false
#define FULL_TPCC true
// Some of the transactions read the data but never use them.
// If TPCC_ACCESS_ALL == fales, then these parts of the transactions
// are not modeled.
#define TPCC_ACCESS_ALL 			false
#define WH_UPDATE					true
#define NUM_WH 16
//
enum TPCCTxnType {TPCC_ALL,
    TPCC_PAYMENT,
    TPCC_NEW_ORDER,
    TPCC_ORDER_STATUS,
    TPCC_DELIVERY,
    TPCC_STOCK_LEVEL};
extern enum TPCCTxnType 					g_tpcc_txn_type;

//#define TXN_TYPE					TPCC_ALL
#define PERC_PAYMENT 0.0
#define FIRSTNAME_MINLEN 			8
#define FIRSTNAME_LEN 				16
#define LASTNAME_LEN 				16

#define DIST_PER_WARE				10

/***********************************************/
// TODO centralized CC management.
/***********************************************/
#define MAX_LOCK_CNT				(20 * THREAD_CNT)
#define TSTAB_SIZE                  50 * THREAD_CNT
#define TSTAB_FREE                  TSTAB_SIZE
#define TSREQ_FREE                  4 * TSTAB_FREE
#define MVHIS_FREE                  4 * TSTAB_FREE
#define SPIN                        false

/***********************************************/
// Test cases
/***********************************************/
#define TEST_ALL					true
enum TestCases {
    READ_WRITE,
    CONFLICT
};
extern enum TestCases					g_test_case;
/***********************************************/
// DEBUG info
/***********************************************/
#define WL_VERB						true
#define IDX_VERB					false
#define VERB_ALLOC					true

#define DEBUG_LOCK					false
#define DEBUG_TIMESTAMP				false
#define DEBUG_SYNTH					false
#define DEBUG_ASSERT				false
#define DEBUG_CC					false //true

/***********************************************/
// Constant
/***********************************************/
// INDEX_STRUCT
#define IDX_HASH 					1
#define IDX_BTREE					2
// WORKLOAD
#define YCSB						1
#define TPCC						2
#define TEST						3
// Concurrency Control Algorithm
#define NO_WAIT						1
#define WAIT_DIE					2
#define DL_DETECT					3
#define TIMESTAMP					4
#define MVCC						5
#define HSTORE						6
#define OCC							7
#define TICTOC						8
#define SILO						9
#define VLL							10
#define HEKATON 					11
//Isolation Levels
#define SERIALIZABLE				1
#define SNAPSHOT					2
#define REPEATABLE_READ				3
// TIMESTAMP allocation method.
#define TS_MUTEX					1
#define TS_CAS						2
#define TS_HW						3
#define TS_CLOCK					4
// Verification type
#define PAGE_VERI                   1
#define MERKLE_TREE                 2
#define DEFERRED_MEMORY             3

#define MSG_SIZE_MAX 4096
#define MSG_TIME_LIMIT 0
#define TPORT_PORT 6000
#define MAX_TPORT_NAME				128
// turn on SGX
#define USE_SGX 0
#define TPORT_TYPE tcp
#define USE_NANOMSG                 1
#define USE_ASYNC_HASH              1
#define USE_LOG 0
#define RPC_SERVER                  "127.0.0.1"
#define LOG_BATCH_SIZE              10

// cache parameters
#define VERIFIED_CACHE_SIZ 1073741824
#define ENABLE_DATA_CACHE true
#define DATA_CACHE_SIZE 1073741824
#define LAZY_OFFLOADING 1
#define BASE_LEASE      100
#define VERI_TYPE DEFERRED_MEMORY
#define VERI_BATCH 1000000000
#define HOT_RECORD_NUM (SYNTH_TABLE_SIZE / 10 / BTREE_ORDER)  // we let 128K records to be hot ones,
// too large results in too long verification.
#define BATCH_MERKLE 1 // calculate the merkle hash in batch to avoid too costly init.
#define BUCKET_FACTOR 1

#if INDEX_STRUCT == IDX_HASH
#define PAGE BucketHeader
#else
#define PAGE bt_node
#endif

#define PRE_LOAD 1
#define PROFILING false
#define TEST_FRESHNESS 0
#define REAL_TIME 0
#define FAST_VERI_CHAIN_ACCESS 1
#define FRESHNESS_STATS_CNT 20000
#define SYNC_VERSION_BATCH 16
#define VACCUM_TRIGGER 128
#define MISS_PENALTY 500000UL

#define TAMPER_PERCENTAGE 0
#define TAMPER_INTERVAL 1000000000
#define TAMPER_RECOVERY 0


// Log queue type
#define LOG_CIRCUL_BUFF 1
#define LOG_STRING_QUEUE 2
#define SMALL_CACHE_SIZE False

#define LOG_QUEUE_TYPE LOG_STRING_QUEUE

#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
#define MAX_LOG_QUEUE_RECORDS 10000
#endif

#define USE_AZURE 0
#endif
