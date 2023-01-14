#ifndef MEM_HELPER_ENC_H_
#define MEM_HELPER_ENC_H_

#include "global_enc_struct.h"


/************************************************/
// STATS helper
/************************************************/
#define INC_STATS_ENC(tid, name, value) \
	if (STATS_ENABLE) \
		stats_enc->_stats[tid]->name += value;

#define INC_TMP_STATS_ENC(tid, name, value) \
	if (STATS_ENABLE) \
		stats_enc->tmp_stats[tid]->name += value;

#define MIN_BEGIN_TS() \
    stats_enc->get_min_begin_ts()

#define INC_GLOB_STATS_ENC(name, value) \
	if (STATS_ENABLE) \
		stats_enc->name += value;

/************************************************/
// malloc helper
/************************************************/
// In order to avoid false sharing, any unshared read/write array residing on the same 
// cache line should be modified to be read only array with pointers to thread local data block.
// TODO. in order to have per-thread malloc, this needs to be modified !!!

#define ARR_PTR_MULTI_ENC(type, name, size, scale) \
	name = new type * [size]; \
	if (g_part_alloc_enc || THREAD_ALLOC) { \
		for (UInt32 i = 0; i < size; i ++) {\
			UInt32 padsize = sizeof(type) * (scale); \
			if (g_mem_pad_enc && padsize % CL_SIZE != 0) \
				padsize += CL_SIZE - padsize % CL_SIZE; \
			name[i] = (type *) malloc(padsize); \
			for (UInt32 j = 0; j < scale; j++) \
				new (&name[i][j]) type(); \
		}\
	} else { \
		for (UInt32 i = 0; i < size; i++) \
			name[i] = new type[scale]; \
	}

#define ARR_PTR_ENC(type, name, size) \
	ARR_PTR_MULTI_ENC(type, name, size, 1)

#define M_ASSERT_ENC(cond, ...) \
	if (!(cond)) {\
		assert(false);\
	}

#define ARR_PTR_INIT_ENC(type, name, size, value) \
	name = new type * [size]; \
	if (g_part_alloc) { \
		for (UInt32 i = 0; i < size; i ++) {\
			int padsize = sizeof(type); \
			if (g_mem_pad && padsize % CL_SIZE != 0) \
				padsize += CL_SIZE - padsize % CL_SIZE; \
			name[i] = (type *) malloc(padsize); \
			new (name[i]) type(); \
		}\
	} else \
		for (UInt32 i = 0; i < size; i++) \
			name[i] = new type; \
	for (UInt32 i = 0; i < size; i++) \
		*name[i] = value; \

#endif