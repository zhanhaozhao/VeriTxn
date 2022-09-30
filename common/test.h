#ifndef TEST_H_
#define TEST_H_

// #include "global.h"
// #include "global_struct.h"
#include "helper.h"

// #include "txn.h"
#include "wl.h"

class TestWorkload : public workload
{
public:
	RC init();
	RC init_table();
	RC init_schema(const char * schema_file);
	// RC get_txn_man(txn_man *& txn_manager, thread_t * h_thd);
	void summarize();
	void tick() { time = get_sys_clock(); };
	INDEX * the_index;
	table_t * the_table;
private:
	uint64_t time;
};

#endif
