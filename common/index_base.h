#ifndef INDEX_BASE_H_
#define INDEX_BASE_H_

#include "global_common.h"
#include "common/helper.h"
#include "common/table.h"

// class table_t;
class index_btree;
class IndexHash;

// principal index structure. The workload may decide to use a different 
// index structure for specific purposes. (e.g. non-primary key access should use hash)
#if (INDEX_STRUCT == IDX_BTREE)
#define INDEX		index_btree
#else  // IDX_HASH
#define INDEX		IndexHash
#endif

class index_base {
public:
	virtual RC 			init() { return RCOK; };
	virtual RC 			init(uint64_t size) { return RCOK; };
	virtual ~index_base() = default;

	virtual bool 		index_exist(idx_key_t key)=0; // check if the key exist.

	virtual RC 			index_insert(idx_key_t key, 
							itemid_t * item, 
							int part_id=-1)=0;

	virtual RC	 		index_read(idx_key_t key, 
							itemid_t * &item,
							int part_id=-1)=0;
	
	virtual RC	 		index_read(idx_key_t key, 
							itemid_t * &item,
							int part_id=-1, int thd_id=0)=0;

	// TODO implement index_remove
	virtual RC 			index_remove(idx_key_t key) { return RCOK; };
	
	// the index in on "table". The key is the merged key of "fields"
	table_t * 			table;
};

#endif