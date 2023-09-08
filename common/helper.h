#ifndef HELPER_H_
#define HELPER_H_

#include <stdlib.h>
// #include <iostream>
#include <stdint.h>
// #include "global.h"
#include "config.h"
#include <string.h>

/************************************************/
// atomic operations
/************************************************/
#define ATOM_ADD(dest, value) \
	__sync_fetch_and_add(&(dest), value)
#define ATOM_SUB(dest, value) \
	__sync_fetch_and_sub(&(dest), value)
// returns true if cas is successful
#define ATOM_CAS(dest, oldval, newval) \
	__sync_bool_compare_and_swap(&(dest), oldval, newval)
#define ATOM_ADD_FETCH(dest, value) \
	__sync_add_and_fetch(&(dest), value)
#define ATOM_FETCH_ADD(dest, value) \
	__sync_fetch_and_add(&(dest), value)
#define ATOM_SUB_FETCH(dest, value) \
	__sync_sub_and_fetch(&(dest), value)

#define COMPILER_BARRIER asm volatile("" ::: "memory");
#define PAUSE { __asm__ ( "pause;" ); }
//#define PAUSE usleep(1);

#define ASSERT(cond) assert(cond)

/************************************************/
// mem copy helper
/************************************************/
#define COPY_VAL(v,d,p) \
	memcpy(&v,&d[p],sizeof(v)); \
	p += sizeof(v);

#define COPY_VAL_SIZE(v,d,p,s) \
	memcpy(&v,&d[p],s); \
	p += s;

#define COPY_BUF(d,v,p) \
	memcpy(&((char*)d)[p],(char*)&v,sizeof(v)); \
	p += sizeof(v);

#define COPY_BUF_SIZE(d,v,p,s) \
	memcpy(&((char*)d)[p],(char*)&v,s); \
	p += s;
	
#define WRITE_VAL(f, v) f.write((char *)&v, sizeof(v));

#define WRITE_VAL_SIZE(f, v, s) f.write(v, sizeof(char) * s);
/************************************************/
// STACK helper (push & pop)
/************************************************/
#define STACK_POP(stack, top) { \
	if (stack == NULL) top = NULL; \
	else {	top = stack; 	stack=stack->next; } }
#define STACK_PUSH(stack, entry) {\
	entry->next = stack; stack = entry; }

/************************************************/
// LIST helper (read from head & write to tail)
/************************************************/
#define LIST_GET_HEAD(lhead, ltail, en) {\
	en = lhead; \
	lhead = lhead->next; \
	if (lhead) lhead->prev = NULL; \
	else ltail = NULL; \
	en->next = NULL; }
#define LIST_PUT_TAIL(lhead, ltail, en) {\
	en->next = NULL; \
	en->prev = NULL; \
	if (ltail) { en->prev = ltail; ltail->next = en; ltail = en; } \
	else { lhead = en; ltail = en; }}
#define LIST_INSERT_BEFORE(entry, newentry) { \
	newentry->next = entry; \
	newentry->prev = entry->prev; \
	if (entry->prev) entry->prev->next = newentry; \
	entry->prev = newentry; }
#define LIST_REMOVE(entry) { \
	if (entry->next) entry->next->prev = entry->prev; \
	if (entry->prev) entry->prev->next = entry->next; }
#define LIST_REMOVE_HT(entry, head, tail) { \
	if (entry->next) entry->next->prev = entry->prev; \
	else { assert(entry == tail); tail = entry->prev; } \
	if (entry->prev) entry->prev->next = entry->next; \
	else { assert(entry == head); head = entry->next; } \
}

/////////////////////////////
// packatize helper 
/////////////////////////////
#define PACK(buffer, var, offset) {\
	memcpy(buffer + offset, &var, sizeof(var)); \
	offset += sizeof(var); \
}
#define PACK_SIZE(buffer, ptr, size, offset) {\
    if (size > 0) {\
		memcpy(buffer + offset, ptr, size); \
		offset += size; }}

#define UNPACK(buffer, var, offset) {\
	memcpy(&var, buffer + offset, sizeof(var)); \
	offset += sizeof(var); \
}
#define UNPACK_SIZE(buffer, ptr, size, offset) {\
    if (size > 0) {\
		memcpy(ptr, buffer + offset, size); \
		offset += size; }} 


enum Data_type {DT_table, DT_page, DT_row };
 
class itemid_t {
public:
	itemid_t() { next = nullptr; location = nullptr; valid = false; };
	itemid_t(Data_type type, void * loc) {
        this->type = type;
        this->location = loc;
        this->next = nullptr;
    };
	Data_type type;
	void * location; // points to the table | page | row
	itemid_t * next;
	bool valid;
	void init();
	bool operator==(const itemid_t &other) const;
	bool operator!=(const itemid_t &other) const;
	void operator=(const itemid_t &other);
};

inline int get_thdid_from_txnid(uint64_t txnid) {return txnid % THREAD_CNT;};

// key_to_part() is only for ycsb
uint64_t key_to_part(uint64_t key);
inline uint64_t get_part_id(void * addr) {return ((uint64_t)addr / PAGE_SIZE) % PART_CNT; }
// TODO can the following two functions be merged?
uint64_t merge_idx_key(uint64_t key_cnt, uint64_t * keys);
uint64_t merge_idx_key(uint64_t key1, uint64_t key2);
uint64_t merge_idx_key(uint64_t key1, uint64_t key2, uint64_t key3);

// extern timespec * res;

inline uint64_t get_server_clock() {
#if defined(__i386__)
    uint64_t ret;
    __asm__ __volatile__("rdtsc" : "=A" (ret));
#elif defined(__x86_64__)
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t ret = ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
	ret = (uint64_t) ((double)ret / CPU_FREQ);
#else 
	timespec * tp = new timespec;
    clock_gettime(CLOCK_REALTIME, tp);
    uint64_t ret = tp->tv_sec * 1000000000 + tp->tv_nsec;
#endif
    return ret;
}

inline uint64_t get_sys_clock() {
// #ifndef NOGRAPHITE
// 	static volatile uint64_t fake_clock = 0;
// 	if (warmup_finish)
// 		return CarbonGetTime();   // in ns
// 	else {
// 		return ATOM_ADD_FETCH(fake_clock, 100);
// 	}
// #else
  #if TIME_ENABLE
	return get_server_clock();
  #else
	return 0;
  #endif
// #endif
}

class myrand {
public:
	void init(uint64_t seed);
	uint64_t next();
private:
	uint64_t seed;
};

inline void set_affinity(uint64_t thd_id) {
	return;
	/*
	// TOOD. the following mapping only works for swarm
	// which has 4-socket, 10 physical core per socket, 
	// 80 threads in total with hyper-threading
	uint64_t a = thd_id % 40;
	uint64_t processor_id = a / 10 + (a % 10) * 4;
	processor_id += (thd_id / 40) * 40;
	
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(processor_id, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	*/
}

#endif