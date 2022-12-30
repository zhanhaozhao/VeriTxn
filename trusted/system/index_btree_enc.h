//
// Created by pan on 2022/12/10.
//

#ifndef DBX1000_INDEX_BTREE_ENC_H
#define DBX1000_INDEX_BTREE_ENC_H

#include "common/index_btree.h"
#include "common/global_common.h"
#include "index_base.h"
#include "string"
#include "common/base_row.h"
#include "common/helper.h"
#include "common/lru_cache.h"

class IndexBTEnc;

struct BTNode {
    // TODO bad hack!
    bool is_leaf;
    idx_key_t * keys;   // keys[0] as node ID.
    UInt32 num_keys;
    bool latch;
    pthread_mutex_t locked;
    latch_t latch_type;
    UInt32  share_cnt;

    uint    part;
    uint64_t parent;
    uint64_t *child;
    uint64_t next;
    void** data;
    bt_node *origin;
    uint64_t node_id;
    IndexBTEnc* from;

#if VERI_TYPE == MERKLE_TREE
    uint64_t merkle_hash;
    uint64_t *child_merkle_hash;
    uint64_t hash() const;
#elif VERI_TYPE == PAGE_VERI
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
    //
    void            update_hash(BTNode* c);
    bool            latch;
    int             root_owner_thread;    // merkle tree contend on the root hash and thus no concurrency.
    RC   get_root_latch(int thread_id);
    void release_root_latch(int thread_id);
    RC merkle_update(BTNode *c);
    void up_to_root(BTNode *c);
#endif
    std::string     index_name;
    table_t*        table;

    lru_cache   *_cache;
    RC release_up_cache(BTNode *c);

    RC index_insert(idx_key_t key, itemid_t *item, int part_id);

private:
    // index structures may have part_cnt = 1 or PART_CNT.
    uint64_t part_cnt;
    BTNode*		make_node(bt_node* out, int64_t part);

    RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf, BTNode  *& last_ex);
    RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf);
    UInt32	 	order; // # of keys in a node(for both leaf and non-leaf)
    BTNode** 	roots; // each partition has a different root
#if VERI_TYPE == PAGE_VERI
    uint64_t    **_verify_hash;
#endif
    BTNode *   find_root(uint64_t part_id);

    bool 		latch_node(BTNode * node, latch_t latch_type);
    latch_t		release_latch(BTNode * node);
    RC		 	upgrade_latch(BTNode * node);
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
};


#endif //DBX1000_INDEX_BTREE_ENC_H
