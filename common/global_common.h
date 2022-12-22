#ifndef GLOBAL_COMMON_H_
#define GLOBAL_COMMON_H_

// #include "stdint.h"
#include <iostream>
#include <unistd.h>
// #include <cstddef>
#include <stdlib.h>
#include <cassert>
// #include <stdio.h>
// #include <fstream>
// #include <mm_malloc.h>
// #include <typeinfo>
// #include <list>
#include <map>
// #include <set>
#include <string>
// #include <vector>
// #include <sstream>

// #include "tlibc/pthread.h"
#include "pthread.h"

#include "config.h"

// #include "stats.h"
// #include "mem_alloc.h"
// #include "query.h"

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

typedef uint32_t UInt32;
typedef int32_t SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;
//typedef std::pair<void*, size_t> DFlow;

typedef uint64_t ts_t; // time stamp type



enum RC { RCOK, Commit, Abort, WAIT, ERROR, FINISH};

/* Thread */
typedef uint64_t txnid_t;

/* Txn */
typedef uint64_t txn_t;

/* Table and Row */
typedef uint64_t rid_t; // row id
typedef uint64_t pgid_t; // page id



/* INDEX */
enum latch_t {LATCH_EX, LATCH_SH, LATCH_NONE};
// accessing type determines the latch type on nodes
enum idx_acc_t {INDEX_INSERT, INDEX_READ, INDEX_NONE, INDEX_EX};
typedef uint64_t idx_key_t; // key id for index
typedef uint64_t (*func_ptr)(idx_key_t);	// part_id func_ptr(index_key);

/* general concurrency control */
enum access_t {RD, WR, XP, SCAN};
/* LOCK */
enum lock_t {LOCK_EX, LOCK_SH, LOCK_NONE };
/* TIMESTAMP */
enum TsType {R_REQ, W_REQ, P_REQ, XP_REQ}; 


#define MSG(str, args...) { \
	printf("[%s : %d] " str, __FILE__, __LINE__, args); } \
//	printf(args); }

/************************************************/
// constants
/************************************************/
#ifndef UINT64_MAX
#define UINT64_MAX 		18446744073709551615UL
#endif // UINT64_MAX

#endif