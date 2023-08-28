== Config File ==

dbms benchmark has the following parameters in the config file. Parameters with a * sign should not be changed
  NODE_CNT    : number of nodes in the system
  PART_CNT		: number of logical partitions in the system
  THREAD_CNT	: number of threads running at the same time
  PAGE_SIZE		: memory page size
  CL_SIZE		: cache line size
  WARMUP		: number of transactions to run for warmup

  WORKLOAD		: workload supported (TPCC or YCSB)
  
  THREAD_ALLOC	: per thread allocator
  * MEM_PAD		: enable memory padding to avoid false sharing
  MEM_ALLIGN	: allocated blocks are alligned to MEM_ALLIGN bytes

  PRT_LAT_DISTR	: print out latency distribution of transactions

  CC_ALG		: concurrency control algorithm
  * ROLL_BACK		: roll back the modifications if a transaction aborts
  
  ENABLE_LATCH  : enable latching in btree index
  * CENTRAL_INDEX : centralized index structure
  * CENTRAL_MANAGER	: centralized lock/timestamp manager
  INDEX_STRUCT	: index structure (IDX_BTREE, IDX_HASH)
  BTREE_ORDER	: fanout of each B-tree node

  DL_TIMEOUT_LOOP	: the max waiting time in DL_DETECT. after timeout, deadlock will be detected
  TS_TWR		: enable Thomas Write Rule (TWR) in TIMESTAMP
  HIS_RECYCLE_LEN	: in MVCC, history will be recycled if they are too long
  MAX_WRITE_SET	: the max size of a write set in OCC

  MAX_ROW_PER_TXN	: max number of rows touched per transaction
  QUERY_INTVL	: the rate at which database queries come
  MAX_TXN_PER_PART	: maximum transactions to run per partition
  USE_LOG   : enable durable transaction log
  
  // for YCSB Benchmark
  SYNTH_TABLE_SIZE	: table size
  ZIPF_THETA	: theta in zipfian distribution (rows accessed follow zipfian distribution)
  READ_PERC		: read operation percentage
  WRITE_PERC	: write operation percentage
  SCAN_PERC		: percentage of read/write/scan queries. they should add up to 1.
  SCAN_LEN		: number of rows touched per scan query.
  PART_PER_TXN	: number of logical partitions to touch per transaction
  PERC_MULTI_PART	: percentage of multi-partition transactions
  REQ_PER_QUERY	: number of queries per transaction
  FIRST_PART_LOCAL	: with this being true, the first touched partition is always the local partition
  
  // for TPCC Benchmark
  NUM_WH		: number of warehouses being modeled.
  PERC_PAYMENT	: percentage of payment transactions. Note that this setting is only applicable when `TPCC_ACCESS_ALL` is set to `false`

  DIST_PER_WARE	: number of districts in one warehouse
  MAXITEMS		: number of items modeled
  CUST_PER_DIST	: number of customers per district
  ORD_PER_DIST	: number of orders per district
  FIRSTNAME_LEN	: length of first name
  MIDDLE_LEN	: length of middle name
  LASTNAME_LEN	: length of last name
  TPCC_ACCESS_ALL : enable all accesses in TPCC benchmark
  FULL_TPCC : enable full TPCC

  // for test measurement
  PROFILING   : enable profiling
  TEST_FRESHNESS  : enable freshness calculation
  REAL_TIME : enable real time performance measurement

  // for verification
  USE_SGX             : enable SGX for verification
  VERIFIED_CACHE_SIZ  : size of verified cache
  TAMPER_PERCENTAGE   : percentage of tampered data
  TAMPER_RECOVERY     : enable tamper recovery
  VERI_TYPE           : verification algorithm (DEFERRED_MEMORY, PAGE_VERI, MERKLE_TREE)
  MSG_SIZE_MAX        : buffer size for messages between nodes
  SYNC_VERSION_BATCH  : batch size for verified hash synchronization

  // for cache control
  DATA_CACHE_SIZE     : size of data cache
  ENABLE_DATA_CACHE   : enable data cache

== Key Default Values ==

  NODE_CNT : 4
  THREAD_CNT : 4
  CC_ALG   : OCC
  USE_SGX : 1
  VERIFIED_CACHE_SIZ  : 4 * 1024 * 1024 * 1024
  DATA_CACHE_SIZE     : 4 * 1024 * 1024 * 1024
  ENABLE_DATA_CACHE : true
  VERI_TYPE : PAGE_VERI
  INDEX_STRUCT  : IDX_BTREE
  USE_LOG : 1
  PROFILING   : false
  SYNC_VERSION_BATCH  : 16
  TAMPER_PERCENTAGE : 0
  TAMPER_RECOVERY : 1
  
  SYNTH_TABLE_SIZE  : 10 * 1000 * 1000
  REQ_PER_QUERY : 16
  ZIPF_THETA  : 0.5
  READ_PERC : 0.5
  WRITE_PERC : 0.5

  NUM_WH  : 16
  PERC_PAYMENT : 0.5
  FULL_TPCC : true
  TPCC_ACCESS_ALL : false
