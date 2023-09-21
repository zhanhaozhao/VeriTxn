//
// Created by pan on 2022/12/10.
//

#ifndef DBX1000_INDEX_BTREE_ENC_H
#define DBX1000_INDEX_BTREE_ENC_H

#include "../../common/index_btree.h"
#include "../../common/global_common.h"
#include "../../common/index_base.h"
#include "string"
#include "../../common/base_row.h"
#include "../../common/helper.h"
#include "../../common/lru_cache.h"
//#include "row_enc.h"

class IndexBTEnc;

#if VERI_TYPE == DEFERRED_MEMORY
struct verify_record;
struct memory_verifier;
#endif

struct BTNode {
    // TODO bad hack!
    bool is_leaf;
    idx_key_t * keys;   // keys[0] as node ID.
    UInt32 num_keys;
    bool latch;
    pthread_mutex_t locked;
    latch_t latch_type;
    UInt32  share_cnt;
    int version;

    uint    part;
    uint64_t parent;
    uint64_t *child;
    uint64_t next;
    void** data;
    bt_node *origin;
    uint64_t node_id;
    IndexBTEnc* from;

    uint64_t get_size() {
        if (is_leaf) return 1024 * num_keys;
        uint64_t  res = (sizeof (*this));
        // pointers and data
        res += sizeof (idx_key_t) * (num_keys+1); // keys
        if (child != nullptr) res += sizeof(uint64_t) * (num_keys+1);
#if VERI_TYPE == MERKLE_TREE
        res += sizeof(uint64_t) * (num_keys+1); // child hashmake
#endif
        // the size of pointed data.
        return res;
    }

#if VERI_TYPE == MERKLE_TREE
    uint64_t merkle_hash;
    uint64_t *child_merkle_hash;
    uint64_t hash() const;
#elif VERI_TYPE == PAGE_VERI
    uint64_t get_hash();
#elif VERI_TYPE == DEFERRED_MEMORY
    uint64_t get_hash();
#endif
};

class IndexBTEnc {
public:
    RC			init(uint64_t part_cnt);
    RC          load_all(std::string index_name);
    BTNode*     load_child(BTNode *cur_node, int i);
    RC			init(uint64_t part_cnt, table_t * table);
    RC	 		index_read(idx_key_t key, itemid_t * &item, int part_id = -1);
    RC	 		index_read(idx_key_t key, itemid_t * &item, int part_id=-1, int thd_id=0);
    RC 			index_next(itemid_t * &item, itemid_t * old, bool samekey = false);
    RC          dfs(BTNode* c);

#if VERI_TYPE == MERKLE_TREE
    void            update_hash(BTNode* c);
    bool            latch;
    RC merkle_update(BTNode *c);
    void up_to_root(BTNode *c);
#elif VERI_TYPE == PAGE_VERI
    uint64_t    **_verify_hash;
#else
    memory_verifier **verifier;
#endif
    std::string     index_name;
    table_t*        table;

    lru_cache   *_cache;
    RC release_up_cache(BTNode *c);

    RC index_insert(idx_key_t key, itemid_t *item, int part_id);
    bool 		latch_node(BTNode * node, latch_t latch_type);
    latch_t		release_latch(BTNode * node);
    RC		 	upgrade_latch(BTNode * node);

private:
    // index structures may have part_cnt = 1 or PART_CNT.
    bool   preloading = false;
    uint64_t part_cnt;
    BTNode*		make_node(bt_node* out, int64_t part);

    RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf, BTNode  *& last_ex);
    RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf);
    UInt32	 	order; // # of keys in a node(for both leaf and non-leaf)
    BTNode** 	roots; // each partition has a different root
    BTNode *   find_root(uint64_t part_id);

    // clean up all the LATCH_EX up tp last_ex
    RC 			cleanup(BTNode * node, BTNode * last_ex);
    uint64_t    _default_bt_veri_hash = 0;

    void flush_out(BTNode *c);

    BTNode *load_next(BTNode *cur_node);

    RC insert_into_leaf(glob_param params, BTNode *leaf, idx_key_t key, itemid_t *item);
    RC split_lf_insert(glob_param params, BTNode *leaf, idx_key_t key, itemid_t *item);
    RC insert_into_parent(glob_param params, BTNode *left, idx_key_t key, BTNode *right);
    void release_cache(BTNode *c);
    RC insert_into_new_root(glob_param params, BTNode *left, idx_key_t key, BTNode *right);
    RC split_nl_insert(glob_param params, BTNode *old_node, UInt32 left_index, idx_key_t key, BTNode *right);
    UInt32 cut(UInt32 length);
    int leaf_has_key(BTNode *leaf, idx_key_t key);
    RC make_node(uint64_t part_id, BTNode *&node, bool isleaf);
    RC add_to_cache(BTNode *old);
    bool inside_data_cache(bt_node *node);

    bool need_tamper_recovery(bt_node *node);
};

#if VERI_TYPE == DEFERRED_MEMORY
// the record used when performing batch verification,
// we defer the generation of timestamp to avoid extra memory cost from ts generator.
struct verify_record {
    uint64_t read_set_hash;
    uint64_t write_set_hash;
    uint64_t timestamp;
    uint64_t old_value_hash;
    bt_node * origin_node;
    IndexBTEnc* from;
    bool locked;

    void get_latch() {
        while ( !ATOM_CAS(locked, false, true) ) {};
    }

    void release_latch() {
        bool ok = ATOM_CAS(locked, true, false);
        assert(ok);
    }

    void init(uint64_t cur_value, bt_node *_origin_node, IndexBTEnc * _from) {
        assert(locked); // for concurrency, the record should be locked before calling init function.
        read_set_hash = 0;
        write_set_hash = cur_value;
        old_value_hash = cur_value;
        origin_node = _origin_node;
        assert(origin_node->node_id > 0);
        from = _from;
        timestamp = 0;
        release_latch();
    }

    // needs latch before.
    bool verify () {
//        return true;
        assert(origin_node->is_leaf);
        assert(PART_CNT == 1);
        auto cur = (BTNode*)(from->_cache->try_load(0, origin_node->node_id));
        uint64_t cur_value;
        if (cur != nullptr) {
            // if cached, no need to read and verify current data.
            get_latch();
            bool ok = (read_set_hash^old_value_hash) == write_set_hash;
            release_latch();
            assert(ok);
            return ok;
        } else {
//            while (!origin_node->from->latch_node(origin_node, LATCH_EX)) {};
            cur_value = origin_node->get_hash();
//            assert(origin_node->from->release_latch(origin_node) == LATCH_EX);
            get_latch();
//            timestamp = 0;
//            old_value_hash = cur_value ^ timestamp;
            auto newh = (cur_value ^ timestamp);
            bool ok = (read_set_hash ^ newh) == write_set_hash;
            release_latch();
            assert(ok);
            return ok;
        }
    };

    // the latch has already been got in the page level, thus no need for further latch.
    void add_read(uint64_t read_value) {
        get_latch();
        read_set_hash ^= read_value ^ timestamp;
        write_set_hash ^= read_value ^ timestamp;
        release_latch();
    }

    void add_write(uint64_t write_value) {
        get_latch();
        read_set_hash ^= old_value_hash;
        timestamp ++;
        write_set_hash ^= write_value ^ timestamp;
        old_value_hash = write_value ^ timestamp;
        release_latch();
    }
};

// memory_verifier a memory_verifier that control the memory verification of a sub-tree.
// page level verification.
struct memory_verifier {
    bool latch;
    BTNode* root;
    verify_record** updates;
    uint64_t _limit;
    uint64_t last_verification;
    bool verifying = false;

    void get_latch() {
        while ( !ATOM_CAS(latch, false, true) ) {};
    }

    void release_latch() {
        bool ok = ATOM_CAS(latch, true, false);
        assert(ok);
    }

    bool verification();
    void init(uint64_t _size, BTNode *root);
    void add_read(uint64_t key, uint64_t read_value, bt_node* origin, IndexBTEnc *from);
    void add_write(uint64_t key, uint64_t write_value, bt_node* origin, IndexBTEnc *from);
};
#endif


#endif //DBX1000_INDEX_BTREE_ENC_H
