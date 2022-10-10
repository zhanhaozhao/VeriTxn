//
// Created by pan on 2022/9/28.
//

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
#include "api.h"

uint64_t compute_hash(std::string const& s) {
    const int p = 31;
    const int m = 1e9 + 9;
    uint64_t hash_value = 0;
    uint64_t p_pow = 1;
    for (char c : s) {
        hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash_value;
}

inline uint64_t string_hash(const std::string& s) {
    return compute_hash(s);
    return uint64_t (std::hash<std::string>{}(s));
}

void test_encoder(const BucketHeader_ENC* x);
void test_encoder(const BucketNode_ENC* x);

RC IndexEnc::init(uint64_t bucket_cnt, int part_cnt) {
    _bucket_cnt_per_part = bucket_cnt / part_cnt;
    _verify_hash = new u_int64_t * [part_cnt];
    _cache = new std::atomic<BucketHeader_ENC*>* [part_cnt];
#ifndef SGX_DISK
    _buckets = new BucketHeader_ENC * [part_cnt];
#endif
    _default_verify_hash = 0;
    for (int i = 0; i < part_cnt; i++) {
        // _verify_hash[i] = (u_int64_t *) aligned_alloc(64, sizeof(u_int64_t) * _bucket_cnt_per_part);
        _verify_hash[i] = (u_int64_t *) malloc(sizeof(u_int64_t) * _bucket_cnt_per_part);
        _cache[i] = new std::atomic<BucketHeader_ENC*> [_bucket_cnt_per_part];
#ifndef SGX_DISK
        // _buckets[i] = (BucketHeader_ENC *) aligned_alloc(64, sizeof(BucketHeader_ENC) * _bucket_cnt_per_part);
        _buckets[i] = (BucketHeader_ENC *) malloc(sizeof(BucketHeader_ENC) * _bucket_cnt_per_part);
#endif
        for (uint32_t n = 0; n < _bucket_cnt_per_part; n ++) {
#ifndef SGX_DISK
            _buckets[i][n].init();
#endif
            _cache[i][n] = nullptr;
            _verify_hash[i][n] = _default_verify_hash;
        }
    }
    return RCOK;
}

RC
IndexEnc::init(int part_cnt, table_t * table, uint64_t bucket_cnt) {
    init(bucket_cnt, part_cnt);
    return RCOK;
}

bool IndexEnc::index_exist(idx_key_t key) {
    assert(false);
}

void
IndexEnc::get_latch(BucketHeader_ENC * bucket) {
    while (!ATOM_CAS(bucket->locked, false, true)) {}
}

void
IndexEnc::release_latch(BucketHeader_ENC * bucket) {
    bool ok = ATOM_CAS(bucket->locked, true, false);
    assert(ok);
}


RC IndexEnc::index_insert(idx_key_t key, itemid_t * item, int part_id) {
    RC rc = RCOK;
    uint64_t bkt_idx = hash(key);
    assert(bkt_idx < _bucket_cnt_per_part);
//    assert(false);
    BucketHeader_ENC * cur_bkt = load_bucket(index_name, part_id, bkt_idx);
    // 1. get the ex latch
    get_latch(cur_bkt);

    // 2. update the latch list
    cur_bkt->insert_item(key, item, part_id);

    // 3. flush the bucket to disk and release the memory
    flush_bucket(part_id, bkt_idx, cur_bkt, true);

    // 4. release the latch
    release_latch(cur_bkt);

    return rc;
}

RC IndexEnc::index_read(std::string iname, idx_key_t key, itemid_t * &item, int part_id) {
    uint64_t bkt_idx = hash(key);
    assert(bkt_idx < _bucket_cnt_per_part);
    assert(iname == index_name);
    BucketHeader_ENC * cur_bkt = load_bucket(index_name, part_id, bkt_idx);
    RC rc = RCOK;
    cur_bkt->read_item(key, item);
    flush_bucket(part_id, bkt_idx, cur_bkt, false);
    return rc;
}

//#define DECOUPLE

#ifndef DECOUPLE
#include <index_hash.h>
#include "base_row.h"
#endif

BucketHeader_ENC* IndexEnc::load_bucket(std::string iname, int part_id, uint64_t bkt_idx) {
#ifndef SGX_DISK
//    auto hs = _buckets[part_id][bkt_idx].get_hash();
//    if (_verify_hash[part_id][bkt_idx] == _default_verify_hash) {
//        _verify_hash[part_id][bkt_idx] = hs;
//    }
//    assert (hs == _verify_hash[part_id][bkt_idx]);
    return &_buckets[part_id][bkt_idx];
#else
//    auto * res_bucket = new BucketHeader_ENC;
//    res_bucket->decode(get_bucket_ocall(index, part_id, bkt_idx));
//    if (_verify_hash[part_id][bkt_idx] == _default_verify_hash) {
//        _verify_hash[part_id][bkt_idx] = res_bucket->get_hash();
//    } else {
//        assert (_verify_hash[part_id][bkt_idx] == res_bucket->get_hash());
//    }
//    return res_bucket;
    auto cur = _cache[part_id][bkt_idx].load();
    if (cur == nullptr) {
#ifndef DECOUPLE
        auto out_bkt = &(((IndexHash*)inner_index_map->_indexes[iname])->_buckets[part_id][bkt_idx]);
        auto res_bucket = new BucketHeader_ENC;
        res_bucket->locked = false;
        BucketNode_ENC* last_node = nullptr;
        for (auto it = out_bkt->first_node; it; it = it->next) {
            auto node = new BucketNode_ENC(it->key);
            itemid_t* last_item = nullptr;
            node->next = nullptr;
            for (auto pt = it->items; pt; pt = pt->next) {
                auto old_row = (base_row_t*)pt->location;
                auto new_row = new row_t;
                int n = old_row->table->get_schema()->get_tuple_size();
                new_row->data = new char [n+1];
                memcpy(new_row->data, old_row->data, n+1);
                new_row->table = old_row->table;
                new_row->init_manager(new_row);
                new_row->set_primary_key(old_row->get_primary_key());
                auto new_item = new itemid_t;
                new_item->next = nullptr;
                new_item->location = (void*)new_row;
                new_item->valid = true;
                new_item->type = DT_row;
                if (last_item == nullptr) {
                    node->items = new_item;
                } else {
                    last_item->next = new_item;
                }
                last_item = new_item;
            }
            if (last_node == nullptr) {
                res_bucket->first_node = node;
            } else {
                last_node->next = node;
            }
            last_node = node;
        }
        get_latch(res_bucket);
        BucketHeader_ENC* tmp = nullptr;
        if (!_cache[part_id][bkt_idx].compare_exchange_strong(tmp, res_bucket)) {
            cur = _cache[part_id][bkt_idx].load();
            release_latch(res_bucket);
        } else {
            cur = res_bucket;
            release_latch(res_bucket);
        }
#else
        auto * res_bucket = new BucketHeader_ENC;
        std::string encoded = get_bucket_ocall(index_name, part_id, bkt_idx);
//        if (bkt_idx == 1) {
//            printf("The bucket origin = %s\n", encoded.c_str());
//        }
        res_bucket->decode(encoded);
        get_latch(res_bucket);
        BucketHeader_ENC* tmp = nullptr;
        if (!_cache[part_id][bkt_idx].compare_exchange_strong(tmp, res_bucket)) {
            cur = _cache[part_id][bkt_idx].load();
            release_latch(res_bucket);
        } else {
            cur = res_bucket;
            if (_verify_hash[part_id][bkt_idx] == _default_verify_hash) {
                _verify_hash[part_id][bkt_idx] = string_hash(encoded);
            } else {
//                assert(_verify_hash[part_id][bkt_idx] == string_hash(encoded));
            }
            assert(_verify_hash[part_id][bkt_idx] == string_hash(encoded));
            release_latch(res_bucket);
//            printf("The encoded value = %s\n", sh.c_str());
//            if (_verify_hash[part_id][bkt_idx] != string_hash(sh)) {
//                printf("The data strings are %s and %s\n", beginh.c_str(), sh.c_str());
//            }
        }
    }
//    if (bkt_idx == 1) {
//        printf("The bucket id = %s\n", cur->encode().c_str());
//    }
    return cur;
#endif
    }
#endif
    return cur;
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
    RC rc = RCOK;
    // 1. get the sh latch
//	get_latch(cur_bkt);
    if (cur_bkt == nullptr) {   // no bucket loaded.
        assert(false);
        return Abort;
    }
    cur_bkt->read_item(key, item);
//    assert (item->location != nullptr);
    // 3. release the latch
//	release_latch(cur_bkt);
    flush_bucket(part_id, bkt_idx, cur_bkt, false);
    return rc;
}

//DFlow IndexEnc::load_disk(int part_id, uint64_t bkt_idx) {
//    return get_bucket_disk(part_id, bkt_idx);
//}

//void IndexEnc::flush_disk(int part_id, uint64_t bkt_idx, const DFlow &value) {
//    put_bucket_disk(part_id, bkt_idx, value);
//}

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
//    if (cur_node == nullptr) {
//        item = nullptr;
//        return;
//    }
    // , "Key does not exist!"
    assert(cur_node->key == key);
    
    item = cur_node->items;
}

uint64_t BucketHeader_ENC::get_hash() const {
    return string_hash(encode());
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