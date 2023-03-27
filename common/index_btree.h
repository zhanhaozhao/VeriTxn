#ifndef INDEX_BTREE_H_
#define INDEX_BTREE_H_

// #include "global_common.h"
// #include "common/helper.h"
#include "../../common/index_base.h"
#include "string"
// #include "common/helper.h"
#include "base_row.h"

class index_btree;

typedef struct bt_node {
   	void ** pointers; // for non-leaf nodes, point to bt_nodes
   	uint64_t node_id;
	bool is_leaf;
	idx_key_t * keys;
	bt_node * parent;
	UInt32 num_keys;
	bt_node * next;
	bool latch;
#if VERI_TYPE == MERKLE_TREE
    uint64_t merkle_hash;
    uint64_t *child_merkle_hash;
    uint64_t hash() const;
#elif VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
    uint64_t get_hash() {
        uint64_t res = 0ULL;
        for (UInt32 i=0;i<num_keys;i++) {
            res ^= keys[i];
        }
        if (is_leaf) {
            for (UInt32 i=0;i<num_keys;i++) {
                auto tmp = (base_row_t*)(((itemid_t*) pointers[i])->location);
                res ^= tmp->hash();
            }
        }
        return num_keys;
    }
#endif
    latch_t latch_type;
    UInt32 share_cnt;
    index_btree* from;
    pthread_mutex_t locked;
} bt_node;

struct glob_param {
	uint64_t part_id;
};

class index_btree : public index_base {
public:
	RC			init(uint64_t part_cnt);
	RC			init(uint64_t part_cnt, table_t * table);
	bool 		index_exist(idx_key_t key); // check if the key exist. 
	RC 			index_insert(idx_key_t key, itemid_t * item, int part_id = -1);
	RC	 		index_read(idx_key_t key, itemid_t * &item, int part_id = -1);
    RC	 		index_read(idx_key_t key, itemid_t * &item, int part_id=-1, int thd_id=0);
	RC	 		index_read(idx_key_t key, itemid_t * &item);
	RC 			index_next(uint64_t thd_id, itemid_t * &item, bool samekey = false);
    char*     index_name;
    bt_node ** 	roots; // each partition has a different root
    bool 		latch_node(bt_node * node, latch_t latch_type) {
        if (!ENABLE_LATCH)
            return true;
        bool success = false;
        while ( !ATOM_CAS(node->latch, false, true) ) {}

        latch_t node_latch = node->latch_type;
        if (node_latch == LATCH_NONE ||
            (node_latch == LATCH_SH && latch_type == LATCH_SH)) {
            node->latch_type = latch_type;
            if (node_latch == LATCH_NONE)
                assert(node->share_cnt == 0);
            if (node->latch_type == LATCH_SH)
                node->share_cnt ++;
            success = true;
        }
        else // latch_type incompatible
            success = false;
        bool ok = ATOM_CAS(node->latch, true, false);
        assert(ok);
        return success;
    }
    latch_t		release_latch(bt_node * node) {
        if (!ENABLE_LATCH)
            return LATCH_SH;
        latch_t type = node->latch_type;
//	if ( g_cc_alg != HSTORE )
        while ( !ATOM_CAS(node->latch, false, true) ) {}
//		pthread_mutex_lock(&node->locked);
//		while (!ATOM_CAS(node->locked, false, true)) {}
        assert(node->latch_type != LATCH_NONE);
        if (node->latch_type == LATCH_EX)
            node->latch_type = LATCH_NONE;
        else if (node->latch_type == LATCH_SH) {
            node->share_cnt --;
            if (node->share_cnt == 0)
                node->latch_type = LATCH_NONE;
        }
//	if ( g_cc_alg != HSTORE )
        bool ok = ATOM_CAS(node->latch, true, false);
        assert(ok);
//		pthread_mutex_unlock(&node->locked);
//		assert(ATOM_CAS(node->locked, true, false));
        return type;
    }
    RC		 	upgrade_latch(bt_node * node) {
        if (!ENABLE_LATCH)
            return RCOK;
        bool success = false;
//	if ( g_cc_alg != HSTORE )
        while ( !ATOM_CAS(node->latch, false, true) ) {}
//		pthread_mutex_lock(&node->locked);
//		while (!ATOM_CAS(node->locked, false, true)) {}
        assert(node->latch_type == LATCH_SH);
        if (node->share_cnt > 1)
            success = false;
        else { // share_cnt == 1
            success = true;
            node->latch_type = LATCH_EX;
            node->share_cnt = 0;
        }

//	if ( g_cc_alg != HSTORE )
        bool ok = ATOM_CAS(node->latch, true, false);
        assert(ok);
//		pthread_mutex_unlock(&node->locked);
//		assert( ATOM_CAS(node->locked, true, false) );
        if (success) return RCOK;
        else return Abort;
    }

#if VERI_TYPE == MERKLE_TREE
    void        update_hash(bt_node * c);
    void up_to_root(bt_node *c);
    RC calculate_hash();
#endif

    RC dfs(bt_node *c);

    uint64_t btree_node_id;
private:
	// index structures may have part_cnt = 1 or PART_CNT.
	uint64_t part_cnt;

    RC			make_lf(uint64_t part_id, bt_node *& node);
	RC			make_nl(uint64_t part_id, bt_node *& node);
	RC		 	make_node(uint64_t part_id, bt_node *& node);
	
	RC 			start_new_tree(glob_param params, idx_key_t key, itemid_t * item);
	RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, bt_node *& leaf, bt_node  *& last_ex);
	RC 			find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, bt_node *& leaf);
	RC			insert_into_leaf(glob_param params, bt_node * leaf, idx_key_t key, itemid_t * item);
	// handle split
	RC 			split_lf_insert(glob_param params, bt_node * leaf, idx_key_t key, itemid_t * item);
	RC 			split_nl_insert(glob_param params, bt_node * node, UInt32 left_index, idx_key_t key, bt_node * right);
	RC 			insert_into_parent(glob_param params, bt_node * left, idx_key_t key, bt_node * right);
	RC 			insert_into_new_root(glob_param params, bt_node * left, idx_key_t key, bt_node * right);

	int			leaf_has_key(bt_node * leaf, idx_key_t key);
	
	UInt32 		cut(UInt32 length);
	UInt32	 	order; // # of keys in a node(for both leaf and non-leaf)
#if VERI_TYPE == PAGE_VERI
    uint64_t    **_verify_hash;
#endif
	bt_node *   find_root(uint64_t part_id);
	// clean up all the LATCH_EX up tp last_ex
	RC 			cleanup(bt_node * node, bt_node * last_ex);

	// the leaf and the idx within the leaf that the thread last accessed.
	bt_node *** cur_leaf_per_thd;
	UInt32 ** 		cur_idx_per_thd;

};

#endif
