// #include <utility>
// #include "global_enc.h"
#include "index_enc.h"
// #include "vector"
#include <cstdlib>
#include <atomic>
// #include <stdlib.h>


#include "mem_helper_enc.h"
#include "row_enc.h"
#include "coder.h"
#include "lru_cache.h"
#include "api.h"

void test_encoder(const BucketHeader_ENC* x);
void test_encoder(const BucketNode_ENC* x);

bool 		latch_node(BucketHeader_ENC * node, latch_t latch_type) {
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
latch_t		release_latch(BucketHeader_ENC * node) {
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

RC IndexEnc::init(uint64_t bucket_cnt, int part_cnt) {
//#if TEST_FRESHNESS == 1
//    freshness_begin_ts_queue = new uint64_t [FRESHNESS_STATS_CNT];    // test the freshness on record 0.
//    // increases with readTS.
//    freshness_read_ts_queue = new uint64_t [FRESHNESS_STATS_CNT];    // test the freshness on record 0.
//    freshness_queue_st = 1;
//    freshness_queue_ed = 0;
//#endif
    _bucket_cnt_per_part = bucket_cnt / part_cnt;
    _verify_hash = new hash_chain ** [part_cnt];
    _flush_time = new ts_t * [part_cnt];
    for (int i = 0;i < part_cnt; i ++) {
        _flush_time[i] = new ts_t [_bucket_cnt_per_part];
    }
    _cache = new lru_cache;
#if WORKLOAD == YCSB
    _cache->init(bucket_cnt, part_cnt, VERIFIED_CACHE_SIZ);
#else
    _cache->init(bucket_cnt, part_cnt, VERIFIED_CACHE_SIZ / 6);
#endif

#ifndef SGX_DISK
    assert(false);
    _buckets = new BucketHeader_ENC * [part_cnt];
#endif
    for (int i = 0; i < part_cnt; i++) {
        // _verify_hash[i] = (u_int64_t *) aligned_alloc(64, sizeof(u_int64_t) * _bucket_cnt_per_part);
        _verify_hash[i] = (hash_chain **) malloc(sizeof(hash_item*) * _bucket_cnt_per_part);
#ifndef SGX_DISK
        // _buckets[i] = (BucketHeader_ENC *) aligned_alloc(64, sizeof(BucketHeader_ENC) * _bucket_cnt_per_part);
        _buckets[i] = (BucketHeader_ENC *) malloc(sizeof(BucketHeader_ENC) * _bucket_cnt_per_part);
#endif
        for (uint32_t n = 0; n < _bucket_cnt_per_part; n ++) {
#ifndef SGX_DISK
            _buckets[i][n].init();
#endif
            _verify_hash[i][n] = new hash_chain;
        }
    }
    return RCOK;
}

RC IndexEnc::init_ts(int part_cnt) {
    for (int i = 0;i < part_cnt; i ++) {
        for (int j = 0; j < _bucket_cnt_per_part; j ++) {
            _flush_time[i][j] = 0;
        }
    }
    _init_ts = get_enc_time();
}

RC
IndexEnc::init(int part_cnt, table_t * table, uint64_t bucket_cnt) {
    init(bucket_cnt, part_cnt);
    return RCOK;
}

bool IndexEnc::index_exist(idx_key_t key) {
    assert(false);
}

//void
//IndexEnc::get_latch(BucketHeader_ENC * bucket) {
//    while (!ATOM_CAS(bucket->locked, false, true)) {}
//}

//void
//IndexEnc::release_latch(BucketHeader_ENC * bucket) {
//    bool ok = ATOM_CAS(bucket->locked, true, false);
//    assert(ok);
//}

int load_cnt = 0;
int miss_cnt = 0;
int max_pg_id = 0;

bool IndexEnc::inside_data_cache(uint64_t pg_id) {
    // we simulate keeping record 0 -- data_cache_size inside caches, since they are mostly accessed.
//    return true;
    load_cnt ++;
    if (pg_id > max_pg_id) {
        max_pg_id = pg_id;
    }
    uint64_t lm = std::uint64_t (_bucket_cnt_per_part / 1024.0 * (1.0*DATA_CACHE_SIZE/SYNTH_TABLE_SIZE + 1.0*VERIFIED_CACHE_SIZ/SYNTH_TABLE_SIZE));
    if (pg_id > lm) {
        miss_cnt ++;
        return false;
    }
    return true;
}

RC IndexEnc::index_insert(idx_key_t key, itemid_t * item, int part_id) {
    assert(false);  // TODO: currently no insert support to the verifiable hash index.
    RC rc = RCOK;
    uint64_t bkt_idx = hash(key);
    assert(bkt_idx < _bucket_cnt_per_part);
//    assert(false);
    BucketHeader_ENC * cur_bkt = load_bucket(index_name, part_id, bkt_idx);
    // 1. get the ex latch
    while (!latch_node(cur_bkt, LATCH_EX));

    // 2. update the latch list
    cur_bkt->insert_item(key, item, part_id);

    // 3. flush the bucket to disk and release the memory
    flush_bucket(part_id, bkt_idx, cur_bkt, true);

    // 4. release the latch
    assert(release_latch(cur_bkt) == LATCH_EX);

//    // 5. release the cache flag.
//    _cache->release(part_id, bkt_idx);

    return rc;
}

RC IndexEnc::index_read(std::string iname, idx_key_t key, itemid_t * &item, int part_id) {
    uint64_t bkt_idx = hash(key);
    assert(bkt_idx < _bucket_cnt_per_part);
    assert(iname == index_name);
    BucketHeader_ENC * cur_bkt = load_bucket(index_name, part_id, bkt_idx);
    if (cur_bkt == nullptr) {
        return ERROR;
    }
    RC rc = RCOK;
    while (!latch_node(cur_bkt, LATCH_SH));
    cur_bkt->read_item(key, item);
    flush_bucket(part_id, bkt_idx, cur_bkt, false);
//    _cache->release(part_id, bkt_idx);
    assert(release_latch(cur_bkt) == LATCH_SH);
    return rc;
}

//#define DECOUPLE

#ifndef DECOUPLE
#include <index_hash.h>
#include "base_row.h"
#endif

#include "common/index_hash.h"
#include "common/base_row.h"
#include "../../common/api.h"

//int flush_count = 0;

// flush_out the data to data cache.
void flush_out(std::string iname, int part_id, uint64_t bkt_idx, BucketHeader_ENC *c) {
    auto res = new BucketHeader;
    res->init();
    c->from->_flush_time[part_id][bkt_idx] = get_enc_time();
//    flush_count ++;
//    if (flush_count % 1000 == 0) {
//        printf("flush = %d\n", flush_count);
//    }
    res->locked = false;
    BucketNode *last_node = nullptr;
    for (auto it = c->first_node; it; it = it->next) {
        auto node = new BucketNode(it->key);
        itemid_t *last_item = nullptr;
        node->next = nullptr;
        for (auto pt = it->items; pt; pt = pt->next) {
            auto old_row = (row_t *) pt->location;
            auto new_row = new base_row_t;
            new_row->init(old_row->table, part_id, old_row->get_row_id());
            int n = old_row->get_tuple_size();
            new_row->data = (char*)malloc((n+1) * sizeof (char ));//new char[n + 1];
            memcpy(new_row->data, old_row->data, n + 1);
            new_row->table = old_row->table;
            new_row->set_primary_key(old_row->get_primary_key());
            auto new_item = new itemid_t;
            new_item->next = nullptr;
            new_item->location = (void *) new_row;
            new_item->valid = true;
            new_item->type = DT_row;
            assert(new_row->hash() == old_row->hash());
            if (last_item == nullptr) {
                node->items = new_item;
            } else {
                last_item->next = new_item;
            }
            last_item = new_item;
        }
        assert(node->hash() == it->hash());
        if (last_node == nullptr) {
            res->first_node = node;
        } else {
            last_node->next = node;
        }
        last_node = node;
    }
    auto idx = (IndexHash *) inner_index_map->_indexes[iname];
    idx->get_latch(&idx->_buckets[part_id][bkt_idx]);   // no concurrent access allowed.
    idx->_buckets[part_id][bkt_idx] = *res;
    c->origin = &idx->_buckets[part_id][bkt_idx];
    assert(c->get_hash() == c->origin->get_hash());
}

int diff_cnt = 0;
int reload_cnt = 0;

BucketHeader_ENC* IndexEnc::load_bucket(std::string iname, int part_id, uint64_t bkt_idx) {
    auto cur = (BucketHeader_ENC*) _cache->try_load(part_id, bkt_idx);
    if (cur != nullptr) {
        uint64_t len = 0, cur_ts = get_enc_time(), rts = 0;
        _verify_hash[part_id][bkt_idx]->get(cur_ts, len, rts);
        INC_GLOB_STATS_ENC(access_cnt, 1);
        INC_GLOB_STATS_ENC(version_chain_length, len);
    }
    if (cur == nullptr) {
        auto res_bucket = new BucketHeader_ENC;
        uint total_rec = 0;
        auto idx = (IndexHash *) inner_index_map->_indexes[iname];
        if (_flush_time[part_id][bkt_idx] == 0) {
            _flush_time[part_id][bkt_idx] = _init_ts;
        }
        if (!preloading && bkt_idx % 100 >= 100-TAMPER_PERCENTAGE &&
            int(get_enc_time() / TAMPER_INTERVAL) != int(_flush_time[part_id][bkt_idx] / TAMPER_INTERVAL)) { //  && get_enc_time()%100 < 10
            // case 1: the page get tampered, load from disk.
//            diff_cnt ++;
#if TAMPER_RECOVERY == 1
            auto start_time = get_enc_time();
            sync_bucket_from_disk(iname, iname.size(), part_id, bkt_idx);
            INC_GLOB_STATS_ENC(time_recover, get_enc_time() - start_time);
#else
//            if (_flush_time[part_id][bkt_idx] != _init_ts) {
//                reload_cnt ++;
//            }
//            if (diff_cnt % 1000 == 0) {
//                printf("recovery count = %d, %d\n", diff_cnt, reload_cnt);
//            }
            return nullptr;
#endif
        }

        if (!inside_data_cache(bkt_idx)) {
            sync_bucket_from_disk(iname, iname.size(), part_id, bkt_idx);
        } else {
#if !ENABLE_DATA_CACHE and USE_LOG and !LOG_RECOVER
        // idx->sync_bucket_from_disk(part_id, bkt_idx);
        // replace with an ocall function
        sync_bucket_from_disk(iname, iname.size(), part_id, bkt_idx);
#endif
        }
        res_bucket->origin = &(idx->_buckets[part_id][bkt_idx]);
        idx->get_latch(res_bucket->origin);
        res_bucket->init();
        res_bucket->from = this;
        res_bucket->bkt = bkt_idx;
        res_bucket->part = part_id;
        res_bucket->locked = false;
        BucketNode_ENC *last_node = nullptr;
        for (auto it = res_bucket->origin->first_node; it; it = it->next) {
            auto node = new BucketNode_ENC(it->key);
            itemid_t *last_item = nullptr;
            node->next = nullptr;
            for (auto pt = it->items; pt; pt = pt->next) {
                auto old_row = (base_row_t *) pt->location;
                auto new_row = new row_t;
                total_rec++;
                new_row->from_page = (void*) res_bucket;
                new_row->offset = 0;
                int n = old_row->table->get_schema()->get_tuple_size();
                new_row->data = new char[n + 1];
                memcpy(new_row->data, old_row->data, n + 1);
                new_row->table = old_row->table;
                new_row->init_manager(new_row);
                new_row->set_row_id(old_row->get_row_id());
                new_row->set_primary_key(old_row->get_primary_key());
                assert(new_row->hash() == old_row->hash());
                auto new_item = new itemid_t;
                new_item->next = nullptr;
                new_item->location = (void *) new_row;
                new_item->valid = true;
                new_item->type = DT_row;
                if (last_item == nullptr) {
                    node->items = new_item;
                } else {
                    last_item->next = new_item;
                }
                last_item = new_item;
            }
            assert(node->hash() == it->hash());
            if (last_node == nullptr) {
                res_bucket->first_node = node;
            } else {
                last_node->next = node;
            }
            last_node = node;
        }
        assert(last_node == nullptr || last_node->next== nullptr);
        auto total_size = total_rec * 1024;
        assert(res_bucket->get_hash() == res_bucket->origin->get_hash());
        void* swapped = nullptr;
        int sw_pt = 0;
        uint64_t sw_bk = 0;
        void * cur_void;
        RC rc = Abort;
        while (rc != RCOK) {
            rc = _cache->load_and_swap(part_id, bkt_idx, (uint64_t) total_size, (void *) res_bucket, swapped, sw_pt, sw_bk, cur_void);
        }
        idx->release_latch(res_bucket->origin);
        assert(rc == RCOK);
        cur = (BucketHeader_ENC*) cur_void;
        if (swapped != nullptr) {
            // if the bucket is flushed outside veri-cache, update the _verify_hash value.
            auto flushed_bkt = (BucketHeader_ENC *)swapped;
#ifdef READ_ONLY
            uint64_t new_hash = flushed_bkt->get_hash();
            if (new_hash == _verify_hash[sw_pt][sw_bk]) {
                _cache->inc_lease(sw_pt, sw_bk);
            } else {
                _cache->reset_lease(sw_pt, sw_bk);
                _verify_hash[sw_pt][sw_bk] = new_hash;
            }
#endif
//            assert(false);
// lazy update of verify hash.
            while (!latch_node(flushed_bkt, LATCH_EX)); // TODO: cannot load this bucket when flushing out.
            _verify_hash[sw_pt][sw_bk]->tail->value = flushed_bkt->get_hash();
            _verify_hash[sw_pt][sw_bk]->tail->commit_ts = get_enc_time();
#if ENABLE_DATA_CACHE
            flush_out(iname, sw_pt, sw_bk, flushed_bkt);
#endif
            delete flushed_bkt;
        }
        if (cur != res_bucket) {    // concurrent index access has loaded the bucket.
            delete res_bucket;
        } else {
            uint64_t len = 0, cur_ts = get_enc_time(), rts = 0;
            if (_verify_hash[part_id][bkt_idx]->empty()) {
                _verify_hash[part_id][bkt_idx]->insert(get_enc_time(), cur->get_hash());
            } else {
                auto hash = cur->get_hash();
                assert(_verify_hash[part_id][bkt_idx]->get(get_enc_time(), len, rts) == cur->get_hash());
#if PRE_LOAD != 1
                INC_GLOB_STATS_ENC(access_cnt, 1);
                INC_GLOB_STATS_ENC(version_chain_length, len);
//                INC_GLOB_STATS_ENC(freshness, cur_ts - rts);
#endif
            }
        }
    }
    return cur;
}

void IndexEnc::release_up_cache(BucketHeader_ENC* c) {
    auto res = _cache->release(c->part, c->bkt);
#if LAZY_OFFLOADING == 1
    assert(res == nullptr);
#else
    if (res != nullptr) {
        auto sw = (BucketHeader_ENC*) res;
        while (!latch_node(sw, LATCH_EX));
        // TODO: cannot load this bucket when flushing out.
        _verify_hash[sw->part][sw->bkt]->tail->value = sw->get_hash();
        _verify_hash[sw->part][sw->bkt]->tail->commit_ts = get_enc_time();
        flush_out(index_name, sw->part, sw->bkt, sw);
        delete sw;
    }
#endif
}

void IndexEnc::flush_bucket(int part_id, uint64_t bkt_idx, BucketHeader_ENC* cur, bool modified) {
#ifndef SGX_DISK
    if (modified) {
//        _verify_hash[part_id][bkt_idx] = cur->get_hash();
        _buckets[part_id][bkt_idx] = *cur;
    }
    return;
#else
    if (modified) {
//        _verify_hash[part_id][bkt_idx] = cur->get_hash();
//        flush_disk(part_id, bkt_idx, cur->encode());
//        test_encoder(cur);
    } else {

    }
//    delete cur;
#endif
}

RC IndexEnc::index_read(std::string iname, idx_key_t key, itemid_t * &item,
                         int part_id, int thd_id) {
    uint64_t bkt_idx = hash(key);
    assert(bkt_idx < _bucket_cnt_per_part);
    assert(iname == index_name);
    BucketHeader_ENC * cur_bkt = load_bucket(index_name, part_id, bkt_idx);
    if (cur_bkt == nullptr) {
        item = nullptr;
        return ERROR;
    }
    RC rc = RCOK;
    // 1. get the sh latch
//	get_latch(cur_bkt);
    while (!latch_node(cur_bkt, LATCH_SH));
    if (cur_bkt == nullptr) {   // no bucket loaded.
        assert(false);
        return Abort;
    }
    cur_bkt->read_item(key, item);

    flush_bucket(part_id, bkt_idx, cur_bkt, false);
    assert(release_latch(cur_bkt) == LATCH_SH);
//    _cache->release(part_id, bkt_idx);
    return rc;
}

/************** BucketHeader_ENC Operations ******************/

void BucketHeader_ENC::init() {
    first_node = nullptr;
    locked = false;
}

void BucketHeader_ENC::insert_item(idx_key_t key,
                               itemid_t * item,
                               int part_id)
{
    BucketNode_ENC * cur_node = first_node;
    BucketNode_ENC * prev_node = nullptr;
    while (cur_node != nullptr) {
        if (cur_node->key == key)
            break;
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    if (cur_node == nullptr) {
        auto * new_node = (BucketNode_ENC *)
                malloc(sizeof(BucketNode_ENC));
        new_node->init(key);
        new_node->items = item;
        if (prev_node != NULL) {
            new_node->next = prev_node->next;
            prev_node->next = new_node;
        } else {
            new_node->next = first_node;
            first_node = new_node;
        }
    } else {
        item->next = cur_node->items;
        cur_node->items = item;
    }
}

void BucketHeader_ENC::read_item(idx_key_t key, itemid_t * &item) const {
    BucketNode_ENC * cur_node = first_node;
    while (cur_node != nullptr) {
        if (cur_node->key == key)
            break;
        cur_node = cur_node->next;
    }
    if (cur_node == nullptr) {
        item = nullptr;
        return;
    }
    // , "Key does not exist!"
    assert(cur_node->key == key);
    
    item = cur_node->items;
}

uint64_t BucketHeader_ENC::get_hash() const {
    uint64_t res = 0;
    BucketNode_ENC * cur_node = first_node;
    while (cur_node != nullptr) {
        res ^= cur_node->hash() ^ cur_node->key;
        cur_node = cur_node->next;
    }
    return res;
}

DFlow BucketHeader_ENC::encode() const {
    std::vector <encoded_record> data;
    BucketNode_ENC * cur_node = first_node;
    while (cur_node != nullptr) {
        data.emplace_back(make_pair(std::to_string(cur_node->key), cur_node->encode()));
        test_encoder(cur_node);
        cur_node = cur_node->next;
    }
    return encode_vec(data);
}

void BucketHeader_ENC::decode(const DFlow & e) {
    std::vector <encoded_record> data = decode_vec(e);
    this->init();
    BucketNode_ENC* last = nullptr;
    for (const auto& it:data) {
        auto tmp = new BucketNode_ENC(stoull(it.first));
        tmp->decode(it.second);
        if (last == NULL) {
            this->first_node = tmp;
            last = tmp;
        } else {
            last->next = tmp;
            last = tmp;
//            this->first_node->next = tmp;
//            this->first_node = tmp;
        }
    }
}

ts_t BucketHeader_ENC::get_ts() const {
    return from->_verify_hash[part][bkt]->get_max_ts();
}

void test_encoder(const BucketHeader_ENC* x) {
#ifdef TEST_C
    auto tmp = new BucketHeader_ENC();
    std::string e = x->encode();
    tmp->decode(e);
//    cout << e << "  and " << tmp->encode() << endl;
    assert(e == tmp->encode());
#endif
}

void test_encoder_row(row_t* x) {
#ifdef TEST_C
    auto tmp = new row_t();
    std::string e = x->encode();
    tmp->decode(e);
//    cout << e << "  and " << tmp->encode() << endl;
    assert(e == tmp->encode());
#endif
}

uint64_t BucketNode_ENC::hash() const {
    uint64_t res = 0;
    itemid_t * it = items;
    res ^= key;
    while (it != nullptr) {
        auto tmp = (row_t*)(it->location);
        res ^= tmp->hash();
        it = it -> next;
    }
    return res;
}

DFlow BucketNode_ENC::encode() const {
    std::vector <encoded_record> data;
    std::string res_items;
    itemid_t * it = items;
    data.emplace_back(std::make_pair(std::to_string(this->key), ""));
    while (it != nullptr) {
        auto tmp = (row_t*)(it->location);
        test_encoder_row(tmp);
        data.emplace_back(std::make_pair(std::to_string(tmp->get_part_id()), tmp->encode()));
        it = it -> next;
    }
    return encode_vec(data);
}

void BucketNode_ENC::decode(const DFlow & e) {
    std::vector <encoded_record> data = decode_vec(e);
    this->init(std::stoull(data[0].first));
    this->items = new itemid_t;
    // this->items->init();
    this->items->valid = false;
	this->items->location = 0;
	this->items->next = NULL;
    int n = data.size();
    itemid_t* last = nullptr;
    for (int i = 1;i < n;i ++) {
        auto * cur_row = new row_t;
        cur_row->decode(data[1].second);
        auto * cur_item = new itemid_t;
        cur_item->location = (void*)cur_row;
        cur_item->valid = true;
        cur_item->type = DT_row;
        cur_item->next = nullptr;
        if (last == nullptr) {
            this->items = cur_item;
            last = cur_item;
        } else {
            last->next = cur_item;
            last = cur_item;
        }
    }
}

void test_encoder(const BucketNode_ENC* x) {
#ifdef TEST_C
    auto tmp = new BucketNode_ENC(x->key);
    std::string e = x->encode();
    tmp->decode(e);
//    cout << e << "  and " << tmp->encode() << endl;
    assert(e == tmp->encode());
#endif
}

// batch here.
void IndexEnc::sync_version(BucketHeader_ENC* c, uint64_t commit_t, uint64_t begin_t, bool updated) {
    if (updated) {
//#if !LAZY_OFFLOADING
//        while (!latch_node(c, LATCH_EX));
//        flush_out(c->from->index_name, c->part, c->bkt, c);
//#if !ENABLE_DATA_CACHE
//        assert(false);  // there is no meaning for single layer cache to offload data.
//#endif
//        assert(release_latch(c) == LATCH_EX);
//#endif
//        printf("synchronizing %lu:(%lu-%lu)\n", c->bkt, begin_t, commit_t);
        _verify_hash[c->part][c->bkt]->tail->commit_ts = commit_t;  // delayed timestamp update;
        uint64_t cur_cnt = ATOM_ADD_FETCH(_verify_hash[c->part][c->bkt]->batch_cnt, 1);
        if (cur_cnt != SYNC_VERSION_BATCH) return;
//        while (!latch_node(c, LATCH_SH));
        async_hash_value(index_name, c->part, c->bkt, c->get_hash(), commit_t);
        _verify_hash[c->part][c->bkt]->batch_cnt = 0;
//        assert(release_latch(c) == LATCH_SH);
    } else {
        INC_GLOB_STATS_ENC(freshness, begin_t - c->get_ts());
        INC_GLOB_STATS_ENC(freshness_cnt, 1);
//        printf("freshness pushing %lu:(%lu-%lu)\n", c->bkt, begin_t, commit_t);
//#if TEST_FRESHNESS == 1
//        if (c->part == 0 && c->bkt == 1 && freshness_queue_ed+1 < FRESHNESS_STATS_CNT) {
//            freshness_queue_ed++;
//            freshness_begin_ts_queue[freshness_queue_ed] = begin_t;
//            freshness_read_ts_queue[freshness_queue_ed] = c->get_ts();
//        }
//#endif
    }
}

uint64_t tot_update_veri = 0;

// version chain can increase with flush out and update verify hash on RO node,
// currently, we only consider to vaccum for the second one.
void IndexEnc::update_verify_hash(int part_id, uint64_t bkt_idx, uint64_t hash, uint64_t ts) {
//    printf("update verify hash of %lu:%lu, %lu\n", bkt_idx, ts, ATOM_ADD_FETCH(tot_update_veri, 1));
    auto bkt = load_bucket(index_name, part_id, bkt_idx);
    if (bkt == nullptr) assert(false);
    while(!latch_node(bkt, LATCH_EX));
//    printf("get latch succeed");
    _verify_hash[part_id][bkt_idx]->insert(ts, hash);
    if ( _verify_hash[part_id][bkt_idx]->size > VACCUM_TRIGGER) {
        uint64_t begin_ts = MIN_BEGIN_TS();
//        printf("%lu - %lu\n",
//               begin_ts, _verify_hash[part_id][bkt_idx]->head->commit_ts);
        _verify_hash[part_id][bkt_idx]->vaccum(begin_ts);
    }
#if USE_LOG == 1 and !LOG_RECOVER
    // TODO: lazy sync from disk to reduce redundant computation.
    assert(!index_name.empty());
    sync_bucket_from_disk(index_name, index_name.size(), part_id, bkt_idx);
#endif
//#if TEST_FRESHNESS == 1
//    if (part_id == 0 && bkt_idx == 1) {
//        // calculate the freshness of data record (0,0)
////        printf("calculating freshness %lu-%lu\n", freshness_queue_st, freshness_queue_ed);
//        while (freshness_queue_st <= freshness_queue_ed &&
//               freshness_read_ts_queue[freshness_queue_st] < ts) {
//            auto read_t = freshness_read_ts_queue[freshness_queue_st];
//            auto begin_t = freshness_begin_ts_queue[freshness_queue_st];
//            assert(read_t <= begin_t);
////            printf("calculating freshness %lu - %lu - %lu\n", freshness_read_ts_queue[freshness_queue_st], ts, freshness_begin_ts_queue[freshness_queue_st]);
//            if (ts <= begin_t) {
//                INC_GLOB_STATS_ENC(freshness, begin_t - ts);
//                INC_GLOB_STATS_ENC(freshness_cnt, 1);
//            } else {
//                INC_GLOB_STATS_ENC(freshness_cnt, 1);
//            }
//            freshness_queue_st ++;
//        }
//    }
//#endif
    assert(release_latch(bkt) == LATCH_EX);
    release_up_cache(bkt);
//    printf("update finish");
}
