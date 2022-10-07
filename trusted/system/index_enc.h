#ifndef INDEX_ENC_H_
#define INDEX_ENC_H_

#include "global_enc.h"
#include "row_enc.h"
#include <atomic>
// #include "mem_helper_enc.h"
// #include "common/helper.h"

#define SGX_DISK    //
//#define TEST_C //

class BucketNode_ENC {
public:
    BucketNode_ENC(idx_key_t key) {	init(key); next = nullptr; items = nullptr;};
    void init(idx_key_t key) {
        this->key = key;
        next = nullptr;
        items = NULL;
    }
    DFlow encode() const;
    void decode(const DFlow & e);
    idx_key_t 		key;
    // The node for the next key
    BucketNode_ENC * 	next;
    // NOTE. The items can be a list of items connected by the next pointer.
    itemid_t * 		items;
};

// BucketHeader_ENC does concurrency control of Hash
class BucketHeader_ENC {
public:
    void init();
    void insert_item(idx_key_t key, itemid_t * item, int part_id);
    void read_item(idx_key_t key, itemid_t * &item) const;
    uint64_t get_hash() const;
    DFlow encode() const;
    void decode(const DFlow & e);
    BucketNode_ENC * 	first_node;
    bool 			locked;
};

class IndexEnc  {
public:
    RC 			init(uint64_t bucket_cnt, int part_cnt);
    RC 			init(int part_cnt,
                       table_t * table,
                       uint64_t bucket_cnt);
    bool 		index_exist(idx_key_t key); // check if the key exist.
    RC 			index_insert(idx_key_t key, itemid_t * item, int part_id=-1);
    // the following call returns a single item
    RC	 		index_read(void * ocall_index, idx_key_t key, itemid_t * &item, int part_id=-1);
    RC	 		index_read(void * ocall_index, idx_key_t key, itemid_t * &item,
                             int part_id=-1, int thd_id=0);
private:
    void get_latch(BucketHeader_ENC * bucket);
    void release_latch(BucketHeader_ENC * bucket);

    // TODO implement more complex hash function
    uint64_t hash(idx_key_t key) {	return key % _bucket_cnt_per_part; }

    uint64_t 			_bucket_cnt_per_part;
    uint64_t 			_default_verify_hash;
    uint64_t**          _verify_hash;
    std::atomic<BucketHeader_ENC*>** _cache;
#ifndef SGX_DISK
    BucketHeader_ENC**      _buckets;
#endif

    BucketHeader_ENC *load_bucket(void * index, int part_id, uint64_t bkt_idx);
    void flush_bucket(int part_id, uint64_t bkt_idx, BucketHeader_ENC *cur, bool modified);
//    static DFlow load_disk(int part_id, uint64_t bkt_idx);
//    static void flush_disk(int part_id, uint64_t bkt_idx, const DFlow & e);
};

#endif