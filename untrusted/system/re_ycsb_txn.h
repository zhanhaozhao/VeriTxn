#ifndef RE_YCSB_TXN_H
#define RE_YCSB_TXN_H


#include "re_txn.h"
#include "ycsb.h"

class re_ycsb_txn_man : public re_txn_man
{
public:
	void init(RPCThread * h_thd, workload * h_wl, uint64_t part_id); 
	RC run_txn(base_query * query);
	void recover_txn(char * log_record, uint64_t tid = (uint64_t)-1); 
private:
	uint64_t row_cnt;
	ycsb_wl * _wl;
	ycsb_query * _query;
};

#endif