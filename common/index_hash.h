#ifndef INDEX_HASH_H_
#define INDEX_HASH_H_

#define DFlow std::string

// #include "global_common.h"
// #include "common/helper.h"
#include "index_base.h"
#include "base_row.h"
// #include "common/helper.h"

//TODO make proper variables private
// each BucketNode contains items sharing the same key
class BucketNode {
public: 
	BucketNode(idx_key_t key) {	init(key); next = nullptr; };
	void init(idx_key_t key) {
		this->key = key;
		next = NULL;
		items = NULL;
	}
    DFlow encode() const;
    void decode(const DFlow & e);
    uint64_t hash() const {
        uint64_t res = 0;
        itemid_t * it = items;
        res ^= key;
        while (it != nullptr) {
            auto tmp = (base_row_t*)(it->location);
            res ^= tmp->hash();
            it = it -> next;
        }
        return res;
    }
    idx_key_t 		key;
	// The node for the next key	
	BucketNode * 	next;
	// NOTE. The items can be a list of items connected by the next pointer. 
	itemid_t * 		items;
};

// BucketHeader does concurrency control of Hash
class BucketHeader {
public:
    void init() {
        node_cnt = 0;
        first_node = NULL;
        locked = false;
    }
	void insert_item(idx_key_t key, itemid_t * item, int part_id);
	void read_item(idx_key_t key, itemid_t * &item, std::string tname);
    DFlow encode() const;
    void decode(const DFlow & e);
	BucketNode * 	first_node;
	uint64_t 		node_cnt;
	bool 			locked;

    uint64_t get_hash() const {
        uint64_t res = 0;
        BucketNode * cur_node = first_node;
        while (cur_node != nullptr) {
            res ^= cur_node->hash() ^ cur_node->key;
            cur_node = cur_node->next;
        }
        return res;
    }
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
    // RC *	sync_bucket_from_disk(int part_id, int bkt_idx);
	char*	index_name;
    BucketHeader ** 	_buckets;
	uint64_t 			_bucket_cnt_per_part;
    void get_latch(BucketHeader * bucket) {
        while (!ATOM_CAS(bucket->locked, false, true)) {}
    }

    void release_latch(BucketHeader * bucket) {
        bool ok = ATOM_CAS(bucket->locked, true, false);
        assert(ok);
    }
private:

	// TODO implement more complex hash function
	uint64_t hash(idx_key_t key) {	return key % _bucket_cnt_per_part; }
	
};

#endif