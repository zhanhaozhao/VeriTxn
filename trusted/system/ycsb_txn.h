#ifndef _YCSB_TXN_H
#define _YCSB_TXN_H


#include "txn.h"
#include "ycsb.h"

class ycsb_txn_man : public txn_man
{
public:
	void init(thread_t * h_thd, workload * h_wl, uint64_t part_id); 
	RC run_txn(base_query * query);
private:
	uint64_t row_cnt;
	ycsb_wl * _wl;
};

#endif