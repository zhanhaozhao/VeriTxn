#ifndef INDEX_HASH_H_
#define INDEX_HASH_H_

#define DFlow std::string

// #include "global_common.h"
// #include "common/helper.h"
#include "index_base.h"
// #include "common/helper.h"

//TODO make proper variables private
// each BucketNode contains items sharing the same key
class BucketNode {
public: 
	BucketNode(idx_key_t key) {	init(key); };
	void init(idx_key_t key) {
		this->key = key;
		next = NULL;
		items = NULL;
	}
    DFlow encode() const;
    void decode(const DFlow & e);
    idx_key_t 		key;
	// The node for the next key	
	BucketNode * 	next;
	// NOTE. The items can be a list of items connected by the next pointer. 
	itemid_t * 		items;
};

// BucketHeader does concurrency control of Hash
class BucketHeader {
public:
	void init();
	void insert_item(idx_key_t key, itemid_t * item, int part_id);
	void read_item(idx_key_t key, itemid_t * &item, std::string tname);
    DFlow encode() const;
    void decode(const DFlow & e);
	BucketNode * 	first_node;
	uint64_t 		node_cnt;
	bool 			locked;
};

// TODO Hash index does not support partition yet.
class IndexHash  : public index_base
{
public:
	RC 			init(uint64_t bucket_cnt, int part_cnt);
	RC 			init(int part_cnt, 
					table_t * table, 
					uint64_t bucket_cnt);
	bool 		index_exist(idx_key_t key); // check if the key exist.
	RC 			index_insert(idx_key_t key, itemid_t * item, int part_id=-1);
	// the following call returns a single item
	RC	 		index_read(idx_key_t key, itemid_t * &item, int part_id=-1);	
	RC	 		index_read(idx_key_t key, itemid_t * &item,
							int part_id=-1, int thd_id=0);
    BucketHeader *	load_bucket(int part_id, int bkt_idx);
	std::string	index_name;
private:
	void get_latch(BucketHeader * bucket);
	void release_latch(BucketHeader * bucket);
	
	// TODO implement more complex hash function
	uint64_t hash(idx_key_t key) {	return key % _bucket_cnt_per_part; }
	
	BucketHeader ** 	_buckets;
	uint64_t 			_bucket_cnt_per_part;
};

#endif