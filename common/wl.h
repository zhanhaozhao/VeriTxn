#ifndef WL_H_
#define WL_H_

#include "global_common.h"
#include "table.h"
#include "index_base.h"

// class base_row_t;
// class table_t;
// class IndexHash;
// class index_btree;
// class Catalog;
// class lock_man;
// class txn_man;
// class thread_t;
// class index_base;
// class Timestamp;
// class Mvcc;

// this is the base class for all workload
class workload
{
public:
	// tables indexed by table name
	std::map<std::string, table_t *> tables;
	std::map<std::string, INDEX *> indexes;

	
	// initialize the tables and indexes.
	virtual RC init();
	virtual RC init_schema(std::string schema_file);
	virtual RC init_table()=0;
	virtual ~workload() = default;
	// virtual RC get_txn_man(txn_man *& txn_manager, thread_t * h_thd)=0;
	
	bool sim_done;
protected:
	void index_insert(std::string index_name, uint64_t key, base_row_t * row);
	void index_insert(INDEX * index, uint64_t key, base_row_t * row, int64_t part_id = -1);
};

#endif