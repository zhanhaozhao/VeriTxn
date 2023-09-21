#include "index_btree_enc.h"
#include "mem_helper_enc.h"
#include "row_enc.h"
#include "atomic"
#include "../../common/index_btree.h"
#include "../../common/base_row.h"
#include "../../common/api.h"

RC IndexBTEnc::init(uint64_t part_cnt) {
    preloading = false;
    this->part_cnt = part_cnt;
    order = BTREE_ORDER;
    // these pointers can be mapped anywhere. They won't be changed
    roots = (BTNode **) malloc(part_cnt * sizeof(BTNode*));
    _cache = new lru_cache;
    _cache->init(BTREE_NODE_NUM, part_cnt, VERIFIED_CACHE_SIZ);
    // "cur_xxx_per_thd" is only for SCAN queries.
    // the index tree of each partition musted be mapped to corresponding l2 slices
#if VERI_TYPE == PAGE_VERI
    _verify_hash = new uint64_t* [part_cnt];
    for (uint64_t i=0;i<part_cnt;i++) {
        _verify_hash[i] = new uint64_t [BTREE_NODE_NUM];
        for (uint64_t j=0;j<BTREE_NODE_NUM;j++) {
            _verify_hash[i][j] = _default_bt_veri_hash;
        }
    }
#elif VERI_TYPE == DEFERRED_MEMORY
    verifier = (memory_verifier **)(malloc(sizeof (memory_verifier *) * part_cnt));
    for (uint64_t i=0;i<part_cnt;i++) {
        verifier[i] = new memory_verifier;
    }
#endif
    return RCOK;
}

bool IndexBTEnc::inside_data_cache(bt_node* node) {
    if (preloading) return true;
    uint64_t lm = BTREE_NODE_NUM / 1024 * (DATA_CACHE_SIZE/SYNTH_TABLE_SIZE + VERIFIED_CACHE_SIZ/SYNTH_TABLE_SIZE);
    if (node->is_leaf && node->keys[0] > lm) {
        return false;
    }
//    if (!node->is_leaf && node->node_id > lm) {
//        return false;
//    }
    return true;
}

bool IndexBTEnc::need_tamper_recovery(bt_node* node) {
    return !preloading && node->node_id % 100 >= 100-TAMPER_PERCENTAGE;
}

#if VERI_TYPE == MERKLE_TREE
uint64_t BTNode::hash() const {
    uint64_t res = 0ULL;
    for (UInt32 i=0;i<num_keys;i++) {
        res ^= keys[i];
    }
    if (is_leaf) {
        res ^= num_keys+1;
        for (UInt32 i=0;i<num_keys;i++) {
            for (auto it = (itemid_t*) data[i];it!= nullptr; it=it->next) {
                auto tmp = (row_t*)(it->location);
                res ^= tmp->hash();
            }
        }
    } else {
        for (UInt32 i=0;i<=num_keys;i++) {
            res ^= child_merkle_hash[i];
        }
    }
    return res;
}
#elif VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
uint64_t BTNode::get_hash() {
    uint64_t res = 0ULL;
    for (UInt32 i=0;i<num_keys;i++) {
        res ^= keys[i];
    }
    if (is_leaf) {
        for (UInt32 i=0;i<num_keys;i++) {
            auto tmp = (row_t*)(((itemid_t*) data[i])->location);
            res ^= tmp->hash();
        }
    }
    return num_keys;
}
#endif

uint64_t flush_num = 0;

// BTree is not suitable for your VeriTXN's verified cache since it needs to cache non-leaf nodes,
// which increases cache miss rate and add-on Ecalls and Ocalls.
void IndexBTEnc::flush_out(BTNode *c) {
    auto cur = ATOM_ADD_FETCH(flush_num, 1);
//    printf("cur flushed out %d:%d:%d\n", cur, c->node_id, c->hash());
#if VERI_TYPE == MERKLE_TREE
    // online updated hash.
    while (!c->origin->from->latch_node(c->origin->from->roots[c->part], LATCH_EX));
    auto pa = load_child(c, -1);
    if (c->parent!=-1 && pa) {
        UInt32 i;
        for (i = 0; i < pa->num_keys; i++) {
            if (c->keys[0] < pa->keys[i])
                break;
        }
        assert(pa->child[i] == c->node_id);
        assert(pa->child_merkle_hash[i] == c->merkle_hash);
        assert(c->origin->merkle_hash = c->merkle_hash);
        c->merkle_hash = c->hash();
        pa->child_merkle_hash[i] = c->merkle_hash;
        _cache->release(pa->part, pa->node_id);
    }
#elif VERI_TYPE == PAGE_VERI
    assert(c->is_leaf); // only need to keep leaf data on time, since no parent hash is maintained.
    _verify_hash[c->part][c->node_id] = c->get_hash();
    while (!c->origin->from->latch_node(c->origin, LATCH_EX));
#elif VERI_TYPE == DEFERRED_MEMORY
    // avoid concurrent load in and flush out.
    c->from->verifier[c->part]->add_write(c->node_id, c->get_hash(), c->origin, c->from);
    while (!c->origin->from->latch_node(c->origin, LATCH_EX));
#endif
    c->origin->num_keys = c->num_keys;
    c->origin->is_leaf = c->is_leaf;
    memcpy(c->origin->keys, c->keys, order * sizeof(idx_key_t));
    auto ln = c->num_keys;
    if (!c->is_leaf) {
        ln++;
    }
    for (UInt32 i = 0;i < ln; i ++) {
        if (c->is_leaf) {
            itemid_t *last_item = nullptr;
            auto cur = (itemid_t*)c->data[i];
            for (auto pt = cur; pt != nullptr; pt = pt->next) {
                auto old_row = (row_t *) pt->location;
                auto new_row = new base_row_t;
                uint n = old_row->table->get_schema()->get_tuple_size();
                new_row->data = new char[n + 1];
                memcpy(new_row->data, old_row->data, sizeof(char) * n);
                new_row->table = old_row->table;
//                new_row->init_manager(new_row);
                new_row->set_primary_key(old_row->get_primary_key());
                new_row->set_row_id(old_row->get_row_id());
                auto new_item = new itemid_t;
                new_item->next = nullptr;
                new_item->location = (void *) new_row;
                new_item->valid = true;
                new_item->type = DT_row;
                assert(new_row->hash() == old_row->hash());
                if (last_item == nullptr) {
                    c->origin->pointers[i] = (void *) new_item;
                } else {
                    last_item->next = new_item;
                }
                last_item = new_item;
            }
        } else {
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
            assert(false);  // non-leaf nodes should not be flushed out in VeriTXN.
#elif VERI_TYPE == MERKLE_TREE
            c->origin->child_merkle_hash[i] = c->child_merkle_hash[i];
//#elif VERI_TYPE == DEFERRED_MEMORY
//            c->from->verifier[c->part]->add_write(c->node_id, c->get_hash(), c->origin, c->from);
#endif
        }
    }

#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
    assert(c->get_hash() == c->origin->get_hash());
    assert(c->origin->from->release_latch(c->origin) == LATCH_EX);
#elif VERI_TYPE == MERKLE_TREE
//    assert(c->hash() == c->origin->hash());
//    c->origin->merkle_hash = c->origin->hash();
    c->origin->merkle_hash = c->hash();
    assert(c->origin->merkle_hash == c->hash());
    assert(c->origin->from->release_latch(c->origin->from->roots[c->part]) == LATCH_EX);
#endif
}

BTNode* IndexBTEnc::load_next(BTNode *cur_node) {
    assert(cur_node->is_leaf == true);
#if VERI_TYPE == DEFERRED_MEMORY
// piggyback verification.
    assert(verifier[cur_node->part]->verification());
#endif
    bt_node* origin_node = cur_node->origin->next;
    uint64_t inner_node_id = cur_node->next;
    if (origin_node == nullptr || inner_node_id == 0) {
        return nullptr;
    }
    auto cur = (BTNode*) _cache->try_load(cur_node->part, inner_node_id);
    if (cur != nullptr) {
        return cur;
    }
    // get latch
    while (!origin_node->from->latch_node(origin_node, LATCH_EX)) {};
    auto new_node = make_node(origin_node, cur_node->part);
    assert(new_node->node_id = inner_node_id);
    void* swapped = nullptr;
    int sw_part = 0;
    uint64_t sw_node_id = 0;
    void *cur_void = nullptr;
    RC rc = Abort;
    while (rc != RCOK) {
        rc = _cache->load_and_swap(cur_node->part, new_node->node_id, new_node->get_size(), (void *) new_node, swapped, sw_part, sw_node_id, cur_void);
    }
    assert (origin_node->from->release_latch(origin_node) == LATCH_EX);
    cur = (BTNode*)cur_void;
    if (rc != RCOK) {
        assert(false);
    }
    else if (swapped != nullptr) {
//        assert(false);
        auto flushed_node = (BTNode *)swapped;

#ifdef READ_ONLY
        uint64_t new_hash = flushed_node->get_hash();
        if (new_hash == _verify_hash[flushed_node->part][flushed_node->node_id]) {
            _cache->inc_lease(flushed_node->part, flushed_node->node_id);
        } else {
            _cache->reset_lease(flushed_node->part, flushed_node->node_id);
            _verify_hash[flushed_node->part][flushed_node->node_id] = new_hash;
        }
#endif
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
        while (!latch_node(flushed_node, LATCH_EX)) {}
        if (flushed_node->is_leaf) {
            flush_out(flushed_node);
        }
        delete flushed_node;
#elif VERI_TYPE == MERKLE_TREE
        merkle_update(flushed_node);
        while (!latch_node(flushed_node, LATCH_EX)) {}
        flush_out(flushed_node);
        delete flushed_node;
        //TODO: support merkle tree delayed hash update.
        // because we use lock-free cache load, the flushed_node could be used by another thread concurrently.
#endif
    }
#if VERI_TYPE == PAGE_VERI
    if (cur != new_node) delete new_node;
    else if (new_node->is_leaf) {
        // only need to verify the leaf node.
        if (_verify_hash[new_node->part][new_node->node_id] == _default_bt_veri_hash)
            _verify_hash[new_node->part][new_node->node_id] = cur->get_hash();
        else assert(_verify_hash[new_node->part][new_node->node_id] == cur->get_hash());
    }
#elif VERI_TYPE == MERKLE_TREE
    if (cur != new_node) delete new_node;
    assert(false);  // merkle tree current does not support next.
#elif VERI_TYPE == DEFERRED_MEMORY
    if (cur != new_node) delete new_node;
    else if (cur->is_leaf) {
        // only need to verify the leaf node.
        verifier[cur->part]->add_read(cur->node_id, cur->get_hash(), cur->origin, cur->from);
    }
#endif
    return cur;
}

BTNode* IndexBTEnc::load_child(BTNode *cur_node, int i) {
    assert(cur_node->is_leaf == false || i == -1);
#if VERI_TYPE == DEFERRED_MEMORY
// piggyback verification.
    assert(verifier[cur_node->part]->verification());
#endif
#if !FULL_TPCC
    bt_node* origin_node = nullptr;
    uint64_t inner_node_id = 0;
    if (i>=0) {
        assert(i <= cur_node->origin->num_keys);
        origin_node = ((bt_node*)cur_node->origin->pointers[i]);
        inner_node_id = cur_node->child[i];
    } else {    // -1 for parent.
        assert(i == -1);
        origin_node = ((bt_node*)cur_node->origin->parent);
        inner_node_id = cur_node->parent;
        if (origin_node!= nullptr && origin_node->node_id <= part_cnt) {
            // root node;
            return roots[origin_node->node_id-1];
        }
    }
    if (origin_node == nullptr || inner_node_id == 0) {
        return nullptr;
    }
#else
    bt_node* origin_node = nullptr;
    uint64_t inner_node_id = 0;
    if (i>=0) {
        if (i <= cur_node->origin->num_keys)
            origin_node = ((bt_node*)cur_node->origin->pointers[i]);
        inner_node_id = cur_node->child[i];
    } else {    // -1 for parent.
        assert(i == -1);
        inner_node_id = cur_node->parent;
        for (int j=0;j<part_cnt;j++) {
            if (inner_node_id == roots[j]->node_id) {
                return roots[j];
            }
        }
    }
    if (inner_node_id == 0) {
        return nullptr;
    }
#endif
#if !FULL_TPCC
    if (origin_node->parent == nullptr) {
        return roots[cur_node->part];
    }
#endif
    auto cur = (BTNode*) _cache->try_load(cur_node->part, inner_node_id);
    if (cur != nullptr) {
        return cur;
    }
    if (!inside_data_cache(origin_node)) {
        sync_bucket_from_disk(index_name, index_name.size(), cur_node->part, inner_node_id);
    }
    if (need_tamper_recovery(origin_node)) {
#if TAMPER_RECOVERY == 1
        auto start_time = get_enc_time();
        sync_bucket_from_disk(index_name, index_name.size(), cur_node->part, inner_node_id);
        INC_GLOB_STATS_ENC(time_recover, get_enc_time() - start_time);
#else
        return nullptr;
#endif
    }
#if !ENABLE_DATA_CACHE
    if (!preloading)
        sync_bucket_from_disk(index_name, index_name.size(), cur_node->part, inner_node_id);
#endif

    // get latch
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
    if (i == -1) return nullptr;
    while (!origin_node->from->latch_node(origin_node, LATCH_EX)) {};
#else
    while (!origin_node->from->latch_node(origin_node->from->roots[cur_node->part], LATCH_EX));
#endif
    auto new_node = make_node(origin_node, cur_node->part);
    assert(new_node->node_id = inner_node_id);
    void* swapped = nullptr;
    int sw_part = 0;
    uint64_t sw_node_id = 0;
    void *cur_void = nullptr;
//    latch_node(new_node, LATCH_EX);
    RC rc = Abort;
    while (rc != RCOK) {
        rc = _cache->load_and_swap(cur_node->part, new_node->node_id, new_node->get_size(), (void *) new_node, swapped, sw_part, sw_node_id, cur_void);
    }
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
    assert (origin_node->from->release_latch(origin_node) == LATCH_EX);
#else
    assert (origin_node->from->release_latch(origin_node->from->roots[cur_node->part]) == LATCH_EX);
#endif

    cur = (BTNode*)cur_void;
    if (rc != RCOK) {
        assert(false);
    }
    else if (swapped != nullptr) {
//        assert(false);
        auto flushed_node = (BTNode *)swapped;

#ifdef READ_ONLY
        uint64_t new_hash = flushed_node->get_hash();
        if (new_hash == _verify_hash[flushed_node->part][flushed_node->node_id]) {
            _cache->inc_lease(flushed_node->part, flushed_node->node_id);
        } else {
            _cache->reset_lease(flushed_node->part, flushed_node->node_id);
            _verify_hash[flushed_node->part][flushed_node->node_id] = new_hash;
        }
#endif
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
        while (!latch_node(flushed_node, LATCH_EX)) {}
        if (flushed_node->is_leaf) {
            flush_out(flushed_node);
        }
        delete flushed_node;
#elif VERI_TYPE == MERKLE_TREE
        while (!latch_node(flushed_node, LATCH_EX)) {}
        merkle_update(flushed_node);
        // because we use lock-free cache load, the flushed_node could be used by another thread concurrently.
        // delayed update in FastVer.
        flush_out(flushed_node);
        delete flushed_node;
#endif
    }
#if VERI_TYPE == PAGE_VERI
    if (cur != new_node) delete new_node;
    else if (new_node->is_leaf) {
        // only need to verify the leaf node.
        if (_verify_hash[new_node->part][new_node->node_id] == _default_bt_veri_hash)
            _verify_hash[new_node->part][new_node->node_id] = cur->get_hash();
        else assert(_verify_hash[new_node->part][new_node->node_id] == cur->get_hash());
    }
#elif VERI_TYPE == MERKLE_TREE
    if (cur != new_node) delete new_node;
    else {
        if (i == -1) {
            // the merkle tree must goes from root to leaf to load data outside cache, the other direction is not safe and needs other verification.
//            assert(cur->hash() == cur->merkle_hash);
        } else {
            // need to verify all nodes.
//            assert(cur->hash() == cur->merkle_hash); could get concurrently changed.
            assert(cur_node->child_merkle_hash[i] == cur->merkle_hash);
        }
//        assert(release_latch(cur) == LATCH_EX);
    }
#elif VERI_TYPE == DEFERRED_MEMORY
    if (cur != new_node) delete new_node;
    else if (cur->is_leaf) {
        // only need to verify the leaf node.
        verifier[cur->part]->add_read(cur->node_id, cur->get_hash(), cur->origin, cur->from);
//        assert(verifier[cur->part]->verification());
    }
#endif
    return cur;
}

#include "../../common/config.h"

RC IndexBTEnc::dfs(BTNode* c) { // force load BTree nodes.
    assert(c != nullptr);
    if (!_cache->can_force_load()) return RCOK; // should leave N/2 space for loading path.
    if (c->is_leaf) {
        _cache->release(c->part, c->node_id);// do not force load leaf nodes.
        return RCOK;
    }
    for (UInt32 i = 0; i <= c->num_keys; i ++) {
        preloading = true;
        auto ch = load_child(c, i);
        preloading = false;
        dfs(ch);
    }
    return RCOK;
}

RC IndexBTEnc::load_all(std::string iname) {
    for (UInt32 part_id = 0; part_id < part_cnt; part_id ++) {
        roots[part_id] = make_node(((index_btree *) inner_index_map->_indexes[iname])->roots[part_id], part_id);
#if VERI_TYPE == DEFERRED_MEMORY
        // todo: support multiple parts.
//        verifier[part_id]->init(std::min(BTREE_NODE_NUM, HOT_RECORD_NUM), roots[part_id]);
        verifier[part_id]->init(BTREE_NODE_NUM, roots[part_id]);
#endif
    }
#if PRE_LOAD == 1
    for (UInt32 part_id = 0; part_id < part_cnt; part_id ++) {
        dfs(roots[part_id]);
    }
#endif
    return RCOK;
}

RC IndexBTEnc::init(uint64_t part_cnt, table_t * table) {
    this->table = table;
    init(part_cnt);
    return RCOK;
}

BTNode * IndexBTEnc::find_root(uint64_t part_id) {
    assert (part_id < part_cnt);
    return roots[part_id];
}

RC
IndexBTEnc::index_read(idx_key_t key,
                        itemid_t *& item,
                        int part_id) {
    return index_read(key, item, 0, part_id);
}

RC IndexBTEnc::index_read(idx_key_t key, itemid_t *& item, int part_id, int thd_id)
{
    RC rc = Abort;
#if VERI_TYPE == MERKLE_TREE
//    rc = get_root_latch(thd_id);
//    if (rc != RCOK) {
//        return rc;
//    }
#endif

    glob_param params;
    assert(part_id != -1);
    params.part_id = part_id;
    BTNode * leaf;
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
    find_leaf(params, key, INDEX_NONE, leaf);
#else
    find_leaf(params, key, INDEX_NONE, leaf);
#endif
    if (leaf == NULL)
        M_ASSERT_ENC(false, "the leaf does not exist!");
    for (UInt32 i = 0; i < leaf->num_keys; i++)
        if (leaf->keys[i] == key) {
            item = (itemid_t *) leaf->data[i];
#if VERI_TYPE == PAGE_VERI or VERI_TYPE == DEFERRED_MEMORY
            release_latch(leaf);
#endif
            return RCOK;
        }
#if !FULL_TPCC
    M_ASSERT_ENC(false, "the key does not exist!");
#endif
    return rc;
}

#if VERI_TYPE == MERKLE_TREE
RC IndexBTEnc::merkle_update(BTNode* c)
{
    up_to_root(c);
    return RCOK;
}
#endif

RC IndexBTEnc::release_up_cache(BTNode* c)
{
    bool first = true;
    for (;c!= nullptr; c = load_child(c, -1)) {
        first = false;
        if (c != roots[c->part]){
            _cache->release(c->part, c->node_id);
            if (!first) _cache->release(c->part, c->node_id);   // the load itself cause cache count++;
        }
    }
}

BTNode* IndexBTEnc::make_node(bt_node* out, int64_t part) {
    BTNode * new_node = (BTNode *) malloc(sizeof(BTNode));
    assert (new_node != nullptr);
    new_node->num_keys = out->num_keys;
    new_node->node_id = out->node_id;
    new_node->from = this;
    if (out->next == nullptr) {
        new_node->next = 0;
    } else {
        new_node->next = out->next->node_id;
    }
    if (out->parent == nullptr) {
        new_node->parent = 0;
    } else {
        new_node->parent = out->parent->node_id;
    }
    new_node->part = part;
    new_node->origin = out;
#if VERI_TYPE == MERKLE_TREE
    new_node->merkle_hash = out->merkle_hash;
#endif
    new_node->is_leaf = out->is_leaf;
    new_node->keys = (idx_key_t *) malloc(order * sizeof(idx_key_t));
    assert (new_node->keys != nullptr);
    new_node->latch = false;
    new_node->latch_type = LATCH_NONE;
    new_node->share_cnt = 0;
    if (new_node->is_leaf) {
        new_node->data = (void**) malloc(order * sizeof(void*));
    } else {
        new_node->data = nullptr;
        new_node->child = (uint64_t*) malloc((order+1) * sizeof(uint64_t));
#if VERI_TYPE == MERKLE_TREE
        new_node->child_merkle_hash = new uint64_t [order];
        for (UInt32 i = 0; i <= new_node->num_keys;i ++) {
            new_node->child_merkle_hash[i] = out->child_merkle_hash[i];
        }
#endif
    }

    for (UInt32 i = 0;i <= new_node->num_keys;i ++) {
        if (i < new_node->num_keys) new_node->keys[i] = out->keys[i];
        if (new_node->is_leaf && i<new_node->num_keys) {
            itemid_t *last_item = nullptr;
            auto cur = (itemid_t*)out->pointers[i];
            for (auto pt = cur; pt; pt = pt->next) {
                auto old_row = (base_row_t *) pt->location;
                auto new_row = new row_t;
                new_row->from_page = (void*) new_node;
                new_row->offset = i;
                uint n = old_row->table->get_schema()->get_tuple_size();
                new_row->data = new char[n + 1];
                memcpy(new_row->data, old_row->data, sizeof (char ) * n);
                new_row->table = old_row->table;
                new_row->init_manager(new_row);
                new_row->set_primary_key(old_row->get_primary_key());
                new_row->set_bucket_id(new_node->node_id);
                new_row->set_row_id(old_row->get_row_id());
                auto new_item = new itemid_t;
                new_item->next = nullptr;
                new_item->location = (void *) new_row;
                new_item->valid = true;
                new_item->type = DT_row;
                assert(new_row->hash() == old_row->hash());
                if (last_item == nullptr) {
                    new_node->data[i] = (void*)new_item;
                } else {
                    last_item->next = new_item;
                }
                last_item = new_item;
            }
        } else if (!new_node->is_leaf) {
            new_node->child[i] = ((bt_node*)out->pointers[i])->node_id;
        }
    }

    assert(new_node->keys[0] == new_node->origin->keys[0]);
    return new_node;
}

bool IndexBTEnc::latch_node(BTNode * node, latch_t latch_type) {
    // TODO latch is disabled
    if (!ENABLE_LATCH)
        return true;
    bool success = false;
    while ( !ATOM_CAS(node->latch, false, true) ) {}

    latch_t node_latch = node->latch_type;
    if (node_latch == LATCH_NONE ||
        (node_latch == LATCH_SH && latch_type == LATCH_SH)) {
        node->latch_type = latch_type;
        if (node_latch == LATCH_NONE)
            M_ASSERT_ENC( (node->share_cnt == 0), "share cnt none 0!" );
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

latch_t IndexBTEnc::release_latch(BTNode * node) {
//#if VERI_TYPE == PAGE_VERI
//    _cache->release(node->part, node->node_id);
//#endif
    if (!ENABLE_LATCH || node == nullptr)
        return LATCH_SH;
    latch_t type = node->latch_type;

    while ( !ATOM_CAS(node->latch, false, true) ) {}

    if (node->latch_type == LATCH_NONE) {
//        assert(false);
        bool ok = ATOM_CAS(node->latch, true, false);
        assert(ok);
        return LATCH_NONE;
    }
    M_ASSERT_ENC((node->latch_type != LATCH_NONE), "release latch fault");
    if (node->latch_type == LATCH_EX)
        node->latch_type = LATCH_NONE;
    else if (node->latch_type == LATCH_SH) {
        assert(node->share_cnt > 0);
        node->share_cnt --;
        if (node->share_cnt == 0) {
            node->latch_type = LATCH_NONE;
        }
    }
    bool ok = ATOM_CAS(node->latch, true, false);
    assert(ok);
    return type;
}

RC IndexBTEnc::upgrade_latch(BTNode * node) {
    if (!ENABLE_LATCH)
        return RCOK;
    bool success = false;
    while ( !ATOM_CAS(node->latch, false, true) ) {}

    M_ASSERT_ENC( (node->latch_type == LATCH_SH), "Error" );
    if (node->share_cnt > 1)
        success = false;
    else { // share_cnt == 1
        success = true;
        node->latch_type = LATCH_EX;
        node->share_cnt = 0;
    }

    bool ok = ATOM_CAS(node->latch, true, false);
    assert(ok);

    if (success) return RCOK;
    else return Abort;
}

RC IndexBTEnc::cleanup(BTNode * node, BTNode * last_ex) {
    if (last_ex != nullptr) {
        do {
            node = load_child(node, -1);
            assert(node != nullptr);
            release_latch(node);
        }
        while (node->node_id != last_ex->node_id);
    }
    return RCOK;
}

RC IndexBTEnc::find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf) {
    BTNode * last_ex = NULL;
    assert(access_type != INDEX_INSERT);
    RC rc = find_leaf(params, key, access_type, leaf, last_ex);
    return rc;
}

RC IndexBTEnc::find_leaf(glob_param params, idx_key_t key, idx_acc_t access_type, BTNode *& leaf, BTNode  *& last_ex)
{
    UInt32 i;
    BTNode * c = find_root(params.part_id);
    assert(c != nullptr);
    BTNode * child;
    if (access_type == INDEX_NONE) {
        while (!c->is_leaf) {
            for (i = 0; i < c->num_keys; i++) {
                if (key < c->keys[i])
                    break;
            }
            c = load_child(c, i); // load pointer function.
        }
        leaf = c;
        return RCOK;
    }
    // key should be inserted into the right side of i
    if (access_type == INDEX_EX) {
        if (!latch_node(c, LATCH_EX))
            return Abort;
        return find_leaf(params, key, INDEX_NONE, leaf, last_ex);
    }
    if (!latch_node(c, LATCH_SH))
        return Abort;
    assert(c->latch_type == LATCH_SH);
    while (!c->is_leaf) {
//        assert(get_part_id(c) == params.part_id);
//        assert(get_part_id(c->keys) == params.part_id);
        assert(c->latch_type == LATCH_SH);
        for (i = 0; i < c->num_keys; i++) {
            if (key < c->keys[i])
                break;
        }
//        assert(key <= c->keys[i] && i <= c->num_keys);
//        assert(c->latch_type == LATCH_SH);
        child = load_child(c, i); // load pointer function.
        child->parent = c->node_id;
        assert(child->parent == c->node_id);
//        assert(key <= child->keys[child->num_keys]);
        assert(c->latch_type == LATCH_SH);
        if (!latch_node(child, LATCH_SH)) {
            release_latch(c);
            cleanup(c, last_ex);
            last_ex = NULL;
            return Abort;
        }
        if (access_type == INDEX_INSERT) {
            if (child->num_keys == order - 1) {
                if (upgrade_latch(c) != RCOK) {
                    release_latch(c);
                    release_latch(child);
                    cleanup(c, last_ex);
                    last_ex = NULL;
                    return Abort;
                }
                if (last_ex == NULL)
                    last_ex = c;
            }
            else {
                assert(c->latch_type != LATCH_NONE);
                cleanup(c, last_ex);
                last_ex = NULL;
                release_latch(c);
            }
        } else
            release_latch(c); // release the LATCH_SH on c
        c = child;
    }
    if (access_type == INDEX_INSERT) {
        if (upgrade_latch(c) != RCOK) {
            release_latch(c);
            cleanup(c, last_ex);
            return Abort;
        }
    }
    leaf = c;
    assert (leaf->is_leaf);
    return RCOK;
}

RC IndexBTEnc::index_next(itemid_t *&item, itemid_t *last, bool samekey) {
    auto old = (row_t*) last->location;
    BTNode* leaf = (BTNode*)old->from_page;
    int idx = old->offset;
    idx_key_t cur_key = leaf->keys[idx];

    auto new_row = old;
    new_row->offset ++;
    if (new_row->offset >= leaf->num_keys) {
        leaf = load_next(leaf);
        new_row->from_page = (void*) leaf;
        new_row->offset = 0;
    }
    if (leaf == NULL)
        item = NULL;
    else {
        assert( leaf->is_leaf );
        if ( samekey && leaf->keys[ new_row->offset ] != cur_key)
            item = NULL;
        else
            item = (itemid_t *) leaf->data[ new_row->offset ];
    }
    return RCOK;
}

#if VERI_TYPE == MERKLE_TREE
void IndexBTEnc::update_hash(BTNode* c) {
    c->merkle_hash = c->hash();
    if (c->parent != 0) {
        auto pa = load_child(c, -1);
        UInt32 i;
        for (i = 0; i < pa->num_keys; i++) {
            if (c->keys[0] < pa->keys[i])
                break;
        }
//        assert(load_child(pa, i) == c);
        pa -> child_merkle_hash[i] = c->merkle_hash;
        _cache->release(pa->part, pa->node_id);
    }
}

void IndexBTEnc::up_to_root(BTNode* c) {
    bool first = true;
    for (;c!= nullptr; c = load_child(c, -1)) {
        update_hash(c);
        if (!first) {
            _cache->release(c->part, c->node_id);
        }
        first = false;
    }
}

//RC IndexBTEnc::get_root_latch(int thread_id) {
//    while (!ATOM_CAS(latch, false, true));
//    if (root_owner_thread == thread_id || root_owner_thread == -1) {
//        root_owner_thread = thread_id;
//        assert(ATOM_CAS(latch, true, false));
//        return RCOK;
//    }
//    assert(ATOM_CAS(latch, true, false));
//    return Abort;
//}

//void IndexBTEnc::release_root_latch(int thread_id) {
//    while (!ATOM_CAS(latch, false, true));
//    // could release for multiple times.
//    root_owner_thread = -1;
//    assert(ATOM_CAS(latch, true, false));
//}
#endif

#ifdef FULL_TPCC
RC IndexBTEnc::index_insert(idx_key_t key, itemid_t * item, int part_id) {
    glob_param params;
    if (WORKLOAD == TPCC) assert(part_id != -1);
    assert(part_id != -1);
    params.part_id = part_id;
    // create a tree if there does not exist one already
    RC rc = RCOK;
    BTNode * root = find_root(params.part_id);
    assert(root != NULL);
    int depth = 0;
    // TODO tree depth < 100
    BTNode * ex_list[100];
    BTNode * leaf = NULL;
    BTNode * last_ex = NULL;
    rc = find_leaf(params, key, INDEX_INSERT, leaf, last_ex);
    if (rc != RCOK) {
        return rc;
    }

    BTNode * tmp_node = leaf;
    if (last_ex != NULL) {
        while (tmp_node != last_ex) {
            //		assert( tmp_node->latch_type == LATCH_EX );
            ex_list[depth++] = tmp_node;
            tmp_node = load_child(tmp_node, -1);
            assert (depth < 100);
        }
        ex_list[depth ++] = last_ex;
    } else
        ex_list[depth++] = leaf;
    // from this point, the required data structures are all latched,
    // so the system should not abort anymore.
//	M_ASSERT(!index_exist(key), "the index does not exist!");
    // insert into btree if the leaf is not full
    if (leaf->num_keys < order - 1 || leaf_has_key(leaf, key) >= 0) {
        rc = insert_into_leaf(params, leaf, key, item);
        // only the leaf should be ex latched.
//		assert( release_latch(leaf) == LATCH_EX );
        int cur_ts = get_enc_time();
        for (int i = 0; i < depth; i++) {
//            release_latch(ex_list[i]);
			assert( release_latch(ex_list[i]) == LATCH_EX );
            ex_list[i]->version = cur_ts;
        }
    }
    else { // split the nodes when necessary
        rc = split_lf_insert(params, leaf, key, item);
        int cur_ts = get_enc_time();
        for (int i = 0; i < depth; i++) {
//            release_latch(ex_list[i]);
            assert(release_latch(ex_list[i]) == LATCH_EX);
            ex_list[i]->version = cur_ts;
        }
    }
#if THREAD_CNT == 1
	assert(leaf->latch_type == LATCH_NONE);
#endif
    return rc;
}

RC IndexBTEnc::insert_into_leaf(glob_param params, BTNode * leaf, idx_key_t key, itemid_t * item) {
    UInt32 i, insertion_point;
    insertion_point = 0;
    int idx = leaf_has_key(leaf, key);
    if (idx >= 0) {
        item->next = (itemid_t *)leaf->data[idx];
        leaf->data[idx] = (void *) item;
#ifdef SEPARATE_MERKLE
        up_to_root(leaf);
#endif
        return RCOK;
    }
    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
        insertion_point++;
    for (i = leaf->num_keys; i > insertion_point; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->data[i] = leaf->data[i - 1];
    }
    leaf->keys[insertion_point] = key;
    leaf->data[insertion_point] = (void *)item;
    leaf->num_keys++;
    M_ASSERT_ENC( (leaf->num_keys < order), "too many keys in leaf" );
#ifdef SEPARATE_MERKLE
    up_to_root(leaf);
#endif
    return RCOK;
}

RC IndexBTEnc::split_lf_insert(glob_param params, BTNode * leaf, idx_key_t key, itemid_t * item) {
    RC rc;
    UInt32 insertion_index, split, i, j;
    idx_key_t new_key;

    uint64_t part_id = params.part_id;
    BTNode * new_leaf;
//	printf("will make_lf(). part_id=%lld, key=%lld\n", part_id, key);
//	pthread_t id = pthread_self();
//	printf("%08x\n", id);
    rc = make_node(part_id, new_leaf, true);
    if (rc != RCOK) return rc;

    M_ASSERT_ENC(leaf->num_keys == order - 1, "trying to split non-full leaf!");

    idx_key_t temp_keys[BTREE_ORDER];
    itemid_t * temp_pointers[BTREE_ORDER];
    insertion_index = 0;
    while (insertion_index < order - 1 && leaf->keys[insertion_index] < key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
        if (j == insertion_index) j++;
//		new_leaf->keys[j] = leaf->keys[i];
//		new_leaf->pointers[j] = (itemid_t *)leaf->pointers[i];
        temp_keys[j] = leaf->keys[i];
        temp_pointers[j] = (itemid_t *)leaf->data[i];
    }
//	new_leaf->keys[insertion_index] = key;
//	new_leaf->pointers[insertion_index] = item;
    temp_keys[insertion_index] = key;
    temp_pointers[insertion_index] = item;

    // leaf is on the left of new_leaf
    split = cut(order - 1);
    leaf->num_keys = 0;
    for (i = 0; i < split; i++) {
//		leaf->pointers[i] = new_leaf->pointers[i];
//		leaf->keys[i] = new_leaf->keys[i];
        leaf->data[i] = temp_pointers[i];
        leaf->keys[i] = temp_keys[i];
        leaf->num_keys++;
        M_ASSERT_ENC( (leaf->num_keys < order), "too many keys in leaf" );
    }
    for (i = split, j = 0; i < order; i++, j++) {
        new_leaf->data[j] = temp_pointers[i];
        new_leaf->keys[j] = temp_keys[i];
        new_leaf->num_keys++;
        M_ASSERT_ENC( (leaf->num_keys < order), "too many keys in leaf" );
    }

    new_leaf->next = leaf->next;
    leaf->next = new_leaf->node_id;

//	new_leaf->pointers[order - 1] = leaf->pointers[order - 1];
//	leaf->pointers[order - 1] = new_leaf;

    for (i = leaf->num_keys; i < order - 1; i++)
        leaf->data[i] = nullptr;
    for (i = new_leaf->num_keys; i < order - 1; i++)
        new_leaf->data[i] = nullptr;

    new_leaf->parent = leaf->parent;
    new_key = new_leaf->keys[0];

    rc = insert_into_parent(params, leaf, new_key, new_leaf);
    return rc;
}

void IndexBTEnc::release_cache(BTNode* c) {
    if (!c) return;
    _cache->release(c->part, c->node_id);
}

RC IndexBTEnc::insert_into_parent(
        glob_param params,
        BTNode * left,
        idx_key_t key,
        BTNode * right) {

    BTNode * parent = load_child(left, -1);

    /* Case: new root. */
    if (parent == NULL)
        return insert_into_new_root(params, left, key, right);

    UInt32 insert_idx = 0;
    while (parent->keys[insert_idx] < key && insert_idx < parent->num_keys)
        insert_idx ++;
    // the parent has enough space, just insert into it
    if (parent->num_keys < order - 1) {
        for (int i = parent->num_keys-1; i >= int(insert_idx); i--) {
            parent->keys[i + 1] = parent->keys[i];
            parent->child[i+2] = parent->child[i+1];
        }
        parent->num_keys ++;
        parent->keys[insert_idx] = key;
        parent->child[insert_idx + 1] = right->node_id;
        return RCOK;
    }

    /* Harder case:  split a node in order
     * to preserve the B+ tree properties.
     */

    return split_nl_insert(params, parent, insert_idx, key, right);
}

RC IndexBTEnc::add_to_cache(BTNode* cur) {
    assert(cur);
    void* swapped = nullptr;
    int sw_part = 0;
    uint64_t sw_node_id = 0;
    void *cur_void = nullptr;
    RC rc = _cache->load_and_swap(cur->part, cur->node_id, cur->get_size(), (void *) cur,
                               swapped, sw_part, sw_node_id, cur_void);
    assert(swapped == nullptr);
    return rc;
}

RC IndexBTEnc::insert_into_new_root(
        glob_param params, BTNode * left, idx_key_t key, BTNode * right)
{
    RC rc;
    uint64_t part_id = params.part_id;
    BTNode * new_root;
    rc = make_node(part_id, new_root, false);
    if (rc != RCOK) return rc;
    new_root->keys[0] = key;
    new_root->child[0] = left->node_id;
    new_root->child[1] = right->node_id;
    new_root->num_keys++;

    M_ASSERT_ENC( (new_root->num_keys < order), "too many keys in leaf" );
    new_root->parent = NULL;
    left->parent = new_root->node_id;
    right->parent = new_root->node_id;
    left->next = right->node_id;

    auto old = roots[part_id];
    assert(old->parent == new_root->node_id);
    assert(rc == RCOK);
    this->roots[part_id] = new_root;
    return RCOK;
}

RC IndexBTEnc::split_nl_insert(
        glob_param params,
        BTNode * old_node,
        UInt32 left_index,
        idx_key_t key,
        BTNode * right)
{
    RC rc;
    uint64_t i, j, split, k_prime;
    BTNode * new_node, * child;
//	idx_key_t * temp_keys;
//	btUInt32 temp_pointers;
    uint64_t part_id = params.part_id;
    rc = make_node(part_id, new_node, false);

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places.
     * Then create a new node and copy half of the
     * keys and pointers to the old node and
     * the other half to the new.
     */

    idx_key_t temp_keys[BTREE_ORDER];
    uint64_t temp_pointers[BTREE_ORDER + 1];
    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = old_node->child[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right->node_id;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */
    split = cut(order);
//	printf("will make_node(). part_id=%lld, key=%lld\n", part_id, key);
    if (rc != RCOK) return rc;

    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++) {
//		old_node->pointers[i] = new_node->pointers[i];
//		old_node->keys[i] = new_node->keys[i];
        old_node->child[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
        M_ASSERT_ENC( (old_node->num_keys < order), "too many keys in leaf" );
    }

    new_node->next = old_node->next;
    old_node->next = new_node->node_id;

    old_node->child[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
//	old_node->pointers[i] = new_node->pointers[i];
//	k_prime = new_node->keys[split - 1];
    for (++i, j = 0; i < order; i++, j++) {
        new_node->child[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
//		new_node->pointers[j] = new_node->pointers[i];
//		new_node->keys[j] = new_node->keys[i];
        new_node->num_keys++;
        M_ASSERT_ENC( (old_node->num_keys < order), "too many keys in leaf" );
    }
    new_node->child[j] = temp_pointers[i];
//	new_node->pointers[j] = new_node->pointers[i];
//	delete temp_pointers;
//	delete temp_keys;
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
        child = load_child(new_node, i);
        child->parent = new_node->node_id;
    }


    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */
    return insert_into_parent(params, old_node, k_prime, new_node);
}

int IndexBTEnc::leaf_has_key(BTNode * leaf, idx_key_t key) {
    for (int i = 0; i < int(leaf->num_keys); i++)
        if (leaf->keys[i] == key) {
            return i;
        }
    return -1;
}

UInt32 IndexBTEnc::cut(UInt32 length) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}

RC IndexBTEnc::make_node(uint64_t part_id, BTNode *& node, bool is_leaf) {
//	printf("make_node(). part_id=%lld\n", part_id);
    BTNode * new_node = (BTNode *) malloc(sizeof(BTNode));
    assert (new_node != NULL);
    new_node->child = NULL;
    new_node->part = part_id;
    new_node->from = this;
    new_node->origin = roots[part_id]->origin;
    new_node->node_id = ATOM_ADD_FETCH(new_node->origin->from->btree_node_id, 1);
    new_node->keys = (idx_key_t *) malloc(order * sizeof(idx_key_t));
    if (is_leaf) {
        new_node->data = (void **) malloc(order * sizeof(void*));
        new_node->child = nullptr;
        assert (new_node->keys != NULL && new_node->data != NULL);
    } else {
        new_node->child = (uint64_t *) malloc(order * sizeof(uint64_t));
        new_node->data = nullptr;
        assert (new_node->keys != NULL && new_node->child != NULL);
    }
    new_node->share_cnt = 0;
    new_node->latch = false;
    new_node->latch_type = LATCH_NONE;
#if VERI_TYPE == MERKLE_TREE
    assert(false);
#endif
    new_node->is_leaf = is_leaf;
    new_node->num_keys = 0;
    new_node->parent = NULL;
    new_node->next = NULL;
    new_node->latch = false;
    new_node->latch_type = LATCH_NONE;
    node = new_node;
    add_to_cache(new_node);
//    void* swapped = nullptr;
//    int sw_part = 0;
//    uint64_t sw_node_id = 0;
//    void *cur_void = nullptr;
//    RC rc = _cache->load_and_swap(new_node->part, new_node->node_id, sizeof (*new_node), (void *) new_node,
//                                  swapped, sw_part, sw_node_id, cur_void);
//    assert(rc == RCOK);
    return RCOK;
}

#if VERI_TYPE == DEFERRED_MEMORY
const uint64_t default_veri_set_value = 0;

bool memory_verifier::verification() {
    if (get_enc_time() - last_verification < VERI_BATCH || !ATOM_CAS(verifying, false, true)) {
        return true;
    }
    // 1. verify the read/write set.
    for (int i = 0;i < _limit; i++) {
        if (updates[i] != nullptr) {
            assert(updates[i]->verify()); // verify the data record.
            updates[i]->get_latch(); // before free the record, get the lock.
            delete updates[i];
            updates[i] = nullptr;
        }
    }
    last_verification = get_enc_time();
    assert(ATOM_CAS(verifying, true, false));
    return true;
    // 2. update the merkle hash at the root.
    // update the root merkle hash.
    // current implementation does not need it since all data is stored on the leaf node, which can be ensured to be integrate by memory verification.
//    root->from->dfs(root);
}

void memory_verifier::init(uint64_t _size, BTNode* _root) {
    // the accepted length for update records.
    root = _root;
    latch = false;
    last_verification = get_enc_time();
    updates = (verify_record**) malloc(sizeof(verify_record*) * _size);
    for (int i = 0;i < _size; i++) {
        updates[i] = nullptr;
    }
    _limit = _size;
}

void memory_verifier::add_read(uint64_t page_key, uint64_t read_value, bt_node* origin, IndexBTEnc *from) {
    uint64_t i = page_key;
//    if (i >= _limit) return;
    if (updates[i] == nullptr) {
        auto new_record = new verify_record;
        new_record->locked = true;
        new_record->init(read_value, origin, from);
        if (!ATOM_CAS(updates[i], nullptr, new_record)) {
            delete new_record;
        }
    }
    assert(updates[i] && i < _limit);
    assert(updates[i]->from == from);
    updates[i]->add_read(read_value);
}

void memory_verifier::add_write(uint64_t page_key, uint64_t write_value, bt_node* origin, IndexBTEnc *from) {
    uint64_t i = page_key;
//    if (i >= _limit) return;
    if (updates[i] == nullptr) {
        auto new_record = new verify_record;
        new_record->locked = true;
        new_record->init(origin->get_hash(), origin, from);
        if (!ATOM_CAS(updates[i], nullptr, new_record)) {
            delete new_record;
        }
    }
    assert(updates[i] && i < _limit);
    assert(updates[i]->from == from);
    updates[i]->add_write(write_value);
}
#endif

#endif