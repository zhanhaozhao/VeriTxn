#ifndef INDEX_ENC_H_
#define INDEX_ENC_H_

#include "global_enc.h"
#include "row_enc.h"
#include "lru_cache.h"
#include <atomic>
#include <index_hash.h>
#include "hash_chain.h"
// #include "mem_helper_enc.h"
// #include "common/helper.h"

#define SGX_DISK    //
//#define TEST_C //

class IndexEnc;

class BucketNode_ENC {
public:
    BucketNode_ENC(idx_key_t key) {	init(key); next = nullptr; items = nullptr;};
    ~BucketNode_ENC() {
        auto next_item = items;
        for (auto it = items; next_item != nullptr; it = next_item) {
            delete (row_t *)it->location;
            next_item = it->next;
            delete it;
        }
    }
    void init(idx_key_t key) {
        this->key = key;
        next = nullptr;
        items = NULL;
    }
    uint64_t hash() const;
    DFlow encode() const;
    void decode(const DFlow & e);
    idx_key_t 		key{};
    // The node for the next key
    BucketNode_ENC * 	next;
    // NOTE. The items can be a list of items connected by the next pointer.
    itemid_t * 		items;
};

// BucketHeader_ENC does concurrency control of Hash
class BucketHeader_ENC {
public:
    BucketHeader_ENC() {
        first_node = nullptr;
        locked = false;
        origin = nullptr;
        part = 0;
        bkt = 0;
        from = nullptr;
        latch_type = LATCH_NONE;
        share_cnt = 0;
        version = get_enc_time();
        latch = false;
    }
    ~BucketHeader_ENC(){
        BucketNode_ENC* next_node = first_node;
        for (auto it = first_node; next_node != nullptr ; it = next_node) {
            next_node = it->next;
            delete it;
        }
    }
    void init();
    void insert_item(idx_key_t key, itemid_t * item, int part_id);
    void read_item(idx_key_t key, itemid_t * &item) const;
    uint64_t get_hash() const;
    ts_t get_ts() const;
    DFlow encode() const;
    void decode(const DFlow & e);
    BucketNode_ENC * 	first_node;
    bool 			locked;
    BucketHeader *  origin;
    int             part;
    int             version;
    uint64_t        bkt{};
    IndexEnc        *from;
    latch_t latch_type;
    UInt32 share_cnt;
    bool latch;
};

class IndexEnc  {
public:
    RC 			init(uint64_t bucket_cnt, int part_cnt);
    RC 			init(int part_cnt,
                       table_t * table,
                       uint64_t bucket_cnt);
    bool 		index_exist(idx_key_t key); // check if the key exist.
    RC 			index_insert(idx_key_t key, itemid_t * item, int part_id=-1);
    void        update_verify_hash(int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts);
    // the following call returns a single item
    RC	 		index_read(std::string iname, idx_key_t key, itemid_t * &item, int part_id=-1);
    RC	 		index_read(std::string iname, idx_key_t key, itemid_t * &item,
                             int part_id=-1, int thd_id=0);
    std::string         index_name;
    BucketHeader_ENC *load_bucket(std::string iname, int part_id, uint64_t bkt_idx);
    void flush_bucket(int part_id, uint64_t bkt_idx, BucketHeader_ENC *cur, bool modified);

//    static DFlow load_disk(int part_id, uint64_t bkt_idx);
//    static void flush_disk(int part_id, uint64_t bkt_idx, const DFlow & e);
    void release_up_cache(BucketHeader_ENC *c);
    void sync_version(BucketHeader_ENC *c, uint64_t commit_t, uint64_t begin_t, bool updated);

    lru_cache*           _cache;
    hash_chain***          _verify_hash;   // make it a chain.
    ts_t **          _flush_time;   // make it a chain.
    bool preloading = true;
    RC init_ts(int part_cnt);

private:
//    void get_latch(BucketHeader_ENC * bucket);
//    void release_latch(BucketHeader_ENC * bucket);
//
    // TODO implement more complex hash function
    uint64_t hash(idx_key_t key) {	return key % _bucket_cnt_per_part; }

    uint64_t 			_bucket_cnt_per_part;
    ts_t _init_ts;
//    hash_chain***          _verify_hash;   // make it a chain.
    // maximum commit timestamp of transactions involving this block.
#ifndef SGX_DISK
    BucketHeader_ENC**      _buckets;
#endif
//#if TEST_FRESHNESS == 1
//    uint64_t * freshness_begin_ts_queue;    // test the freshness on record 0.
//    // increases with readTS.
//    uint64_t * freshness_read_ts_queue;    // test the freshness on record 0.
//    uint64_t freshness_queue_st, freshness_queue_ed;
//#endif

    bool inside_data_cache(uint64_t pg_id);
};

#endif
