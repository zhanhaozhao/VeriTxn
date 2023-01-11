#ifndef TPCC_HELPER_H_
#define TPCC_HELPER_H_

// #include "global.h"
// #include <cassert>
#include <vector>
#include <random>

#include "helper.h"
#include "global_common.h"

const UInt32 max_items = 100000;
const UInt32 cust_per_dist = 3000;

// uint64_t distKey(uint64_t d_id, uint64_t d_w_id);

inline uint64_t distKey(uint64_t d_id, uint64_t d_w_id)  {
	return d_w_id * DIST_PER_WARE + d_id; 
}

// uint64_t custKey(uint64_t c_id, uint64_t c_d_id, uint64_t c_w_id);
inline uint64_t custKey(uint64_t c_id, uint64_t c_d_id, uint64_t c_w_id) {
	return (distKey(c_d_id, c_w_id) * cust_per_dist + c_id);
}

#if TPCC_SMALL
const UInt32 g_cust_per_dist_enc = 2000;
#else
const UInt32 g_cust_per_dist_enc = 3000;
#endif

//uint64_t orderlineKey(uint64_t w_id, uint64_t d_id, uint64_t o_id);
//uint64_t orderPrimaryKey(uint64_t w_id, uint64_t d_id, uint64_t o_id);
#if FULL_TPCC
inline uint64_t neworderKey(int64_t o_id, uint64_t o_d_id, uint64_t o_w_id) {
    uint64_t g_max_orderline = uint64_t(1) << 32;
    return distKey(o_d_id, o_w_id) * g_max_orderline + (g_max_orderline - o_id);
}
#endif

inline uint64_t orderlineKey(uint64_t w_id, uint64_t d_id, uint64_t o_id) {
    return distKey(d_id, w_id) * g_cust_per_dist_enc + o_id;
}

inline uint64_t orderPrimaryKey(uint64_t w_id, uint64_t d_id, uint64_t o_id) {
    return orderlineKey(w_id, d_id, o_id);
}

// non-primary key
// uint64_t custNPKey(char * c_last, uint64_t c_d_id, uint64_t c_w_id);
inline uint64_t custNPKey(char * c_last, uint64_t c_d_id, uint64_t c_w_id) {
	uint64_t key = 0;
	char offset = 'A';
	for (uint32_t i = 0; i < strlen(c_last); i++) 
		key = (key << 2) + (c_last[i] - offset);
	key = key << 3;
	key += c_w_id * DIST_PER_WARE + c_d_id;
	return key;
}

inline uint64_t stockKey(uint64_t s_i_id, uint64_t s_w_id) {
	return s_w_id * max_items + s_i_id;
}

uint64_t Lastname(uint64_t num, char* name);

extern myrand ** tpcc_buffer;

//extern std::vector<std::default_random_engine> tpccdre;
//extern std::vector<std::uniform_int_distribution<uint64_t> > tpccuid;

// return random data from [0, max-1]
uint64_t RAND(uint64_t max, uint64_t thd_id);
// random number from [x, y]
uint64_t URand(uint64_t x, uint64_t y, uint64_t thd_id);
// non-uniform random number
uint64_t NURand(uint64_t A, uint64_t x, uint64_t y, uint64_t thd_id);
// random string with random length beteen min and max.
uint64_t MakeAlphaString(int min, int max, char * str, uint64_t thd_id);
uint64_t MakeNumberString(int min, int max, char* str, uint64_t thd_id);

inline uint64_t wh_to_part(uint64_t wid) {
    assert(PART_CNT <= NUM_WH);
	return wid % PART_CNT;
}

#endif