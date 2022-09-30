#ifndef TEST_TXN_H
#define TEST_TXN_H

#include "txn.h"
#include "test.h"

class TestTxnMan : public txn_man 
{
public:
	void init(thread_t * h_thd, workload * h_wl, uint64_t part_id); 
	RC run_txn(int type, int access_num);
	RC run_txn(base_query * m_query) { assert(false); };
private:
	RC testReadwrite(int access_num);
	RC testConflict(int access_num);
	
	TestWorkload * _wl;
};

#endif
