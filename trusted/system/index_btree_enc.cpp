#include "index_btree_enc.h"
#include "mem_helper_enc.h"
#include "row_enc.h"
#include "atomic"
#include "common/index_btree.h"
#include "common/base_row.h"

RC IndexBTEnc::init(uint64_t part_cnt) {
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
#else
    root_owner_thread = -1;
#endif
    return RCOK;
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
#elif VERI_TYPE == PAGE_VERI
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
#if VERI_TYPE == MERKLE_TREE
    // online updated hash.
    while (!c->origin->from->latch_node(c->origin->from->roots[c->part], LATCH_EX));
    c->merkle_hash = c->hash();
    c->origin->merkle_hash = c->merkle_hash;
    auto pa = load_child(c, -1);
    if (pa) {
        UInt32 i;
        for (i = 0; i < pa->num_keys; i++) {
            if (c->keys[0] < pa->keys[i])
                break;
        }
        assert(pa->child[i] == c->node_id);
        pa->child_merkle_hash[i] = c->merkle_hash;
        pa->merkle_hash = pa->hash();
    }
#elif VERI_TYPE == PAGE_VERI
    assert(c->is_leaf); // only need to keep leaf data on time, since no parent hash is maintained.
    _verify_hash[c->part][c->node_id] = c->get_hash();
    while (!c->origin->from->latch_node(c->origin, LATCH_EX));
#endif
    c->origin->num_keys = c->num_keys;
    c->origin->is_leaf = c->is_leaf;
    memcpy(c->origin->keys, c->keys, order * sizeof(idx_key_t));
//    c->origin->latch = false;
//    c->origin->latch_type = LATCH_NONE;
//    c->origin->share_cnt = 0;
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
#if VERI_TYPE == PAGE_VERI
            assert(false);  // non-leaf nodes should not be flushed out in VeriTXN.
#else
            c->origin->child_merkle_hash[i] = c->child_merkle_hash[i];
#endif
        }
    }

#if VERI_TYPE == PAGE_VERI
    assert(c->get_hash() == c->origin->get_hash());
    assert(c->origin->from->release_latch(c->origin) == LATCH_EX);
#else
//    assert(c->hash() == c->origin->hash());
    assert(c->origin->merkle_hash == c->hash());
    assert(c->origin->from->release_latch(c->origin->from->roots[c->part]) == LATCH_EX);
#endif
//    if (cur % 1000 == 0) {
//        printf("flushed: %lu\n", cur);
//    }
}

BTNode* IndexBTEnc::load_next(BTNode *cur_node) {
    assert(cur_node->is_leaf == true);
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
        rc = _cache->load_and_swap(cur_node->part, new_node->node_id, sizeof (*new_node), (void *) new_node, swapped, sw_part, sw_node_id, cur_void);
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
#if VERI_TYPE == PAGE_VERI
        while (!latch_node(flushed_node, LATCH_EX)) {}
        if (flushed_node->is_leaf) {
            flush_out(flushed_node);
        }
        delete flushed_node;
#elif VERI_TYPE == MERKLE_TREE
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
#endif
    return cur;
}

BTNode* IndexBTEnc::load_child(BTNode *cur_node, int i) {
    assert(cur_node->is_leaf == false || i == -1);
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
    auto cur = (BTNode*) _cache->try_load(cur_node->part, inner_node_id);
    if (cur != nullptr) {
        return cur;
    }
    // get latch
#if VERI_TYPE == PAGE_VERI
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
    RC rc = Abort;
    while (rc != RCOK) {
        rc = _cache->load_and_swap(cur_node->part, new_node->node_id, sizeof (*new_node), (void *) new_node, swapped, sw_part, sw_node_id, cur_void);
    }
#if VERI_TYPE == PAGE_VERI
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
#if VERI_TYPE == PAGE_VERI
        while (!latch_node(flushed_node, LATCH_EX)) {}
        if (flushed_node->is_leaf) {
            flush_out(flushed_node);
        }
        delete flushed_node;
#elif VERI_TYPE == MERKLE_TREE
        while (!latch_node(flushed_node, LATCH_EX)) {}
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
            assert(cur->hash() == cur->merkle_hash);
            assert(cur_node->child_merkle_hash[i] == cur->merkle_hash);
        }
    }
#endif
    return cur;
}

#include "common/config.h"

RC IndexBTEnc::dfs(BTNode* c) { // force load BTree nodes.
    assert(c != nullptr);
    if (c->is_leaf) {
        _cache->release(c->part, c->node_id);// do not force load leaf nodes.
        return RCOK;
    }
    for (UInt32 i = 0; i <= c->num_keys; i ++) {
        dfs(load_child(c, i));
    }
    return RCOK;
}

RC IndexBTEnc::load_all(std::string iname) {
    for (UInt32 part_id = 0; part_id < part_cnt; part_id ++) {
        roots[part_id] = make_node(((index_btree *) inner_index_map->_indexes[iname])->roots[part_id], part_id);
    }
#ifdef PRE_LOAD
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
    rc = get_root_latch(thd_id);
    if (rc != RCOK) {
        return rc;
    }
#endif

    glob_param params;
    assert(part_id != -1);
    params.part_id = part_id;
    BTNode * leaf;
#if VERI_TYPE == PAGE_VERI
    find_leaf(params, key, INDEX_READ, leaf);
#else
    find_leaf(params, key, INDEX_NONE, leaf);
#endif
    if (leaf == NULL)
        M_ASSERT_ENC(false, "the leaf does not exist!");
    for (UInt32 i = 0; i < leaf->num_keys; i++)
        if (leaf->keys[i] == key) {
            item = (itemid_t *) leaf->data[i];
#if VERI_TYPE == PAGE_VERI
            release_latch(leaf);
#endif
            return RCOK;
        }
    M_ASSERT_ENC(false, "the key does not exist!");
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
    if (!ENABLE_LATCH)
        return LATCH_SH;
    latch_t type = node->latch_type;

    while ( !ATOM_CAS(node->latch, false, true) ) {}

    M_ASSERT_ENC((node->latch_type != LATCH_NONE), "release latch fault");
    if (node->latch_type == LATCH_EX)
        node->latch_type = LATCH_NONE;
    else if (node->latch_type == LATCH_SH) {
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
    if (last_ex != NULL) {
        do {
            node = load_child(node, -1);
            release_latch(node);
        }
        while (node != last_ex);
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
    while (!c->is_leaf) {
        assert(get_part_id(c) == params.part_id);
        assert(get_part_id(c->keys) == params.part_id);
        for (i = 0; i < c->num_keys; i++) {
            if (key < c->keys[i])
                break;
        }
//        assert(key <= c->keys[i] && i <= c->num_keys);
        child = load_child(c, i); // load pointer function.
//        assert(key <= child->keys[child->num_keys]);
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
                cleanup(c, last_ex);
                last_ex = NULL;
                release_latch(c);
            }
        } else {
            release_latch(c); // release the LATCH_SH on c
            c = child;
        }
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
        assert(load_child(pa, i) == c);
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

RC IndexBTEnc::get_root_latch(int thread_id) {
    while (!ATOM_CAS(latch, false, true));
    if (root_owner_thread == thread_id || root_owner_thread == -1) {
        root_owner_thread = thread_id;
        assert(ATOM_CAS(latch, true, false));
        return RCOK;
    }
    assert(ATOM_CAS(latch, true, false));
    return Abort;
}

void IndexBTEnc::release_root_latch(int thread_id) {
    while (!ATOM_CAS(latch, false, true));
    // could release for multiple times.
//    assert (root_owner_thread == thread_id || root_owner_thread == -1);
    root_owner_thread = -1;
    assert(ATOM_CAS(latch, true, false));
}
#endif