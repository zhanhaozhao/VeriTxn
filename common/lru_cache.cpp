//
// Created by pan on 2022/10/17.
//

#include "lru_cache.h"

RC lru_cache::init(uint64_t bucket_cnt, int part_cnt, uint64_t siz) {
    locked = false;
    _cache = new cache_node** [part_cnt];
    _cached_cnt = 0;
    _limit = siz;
    for (int i = 0; i < part_cnt; i++) {
        _cache[i] = new cache_node* [bucket_cnt / part_cnt];
        for (uint32_t j = 0; j < bucket_cnt / part_cnt; j ++) {
            _cache[i][j] = nullptr;
        }
    }
    return RCOK;
}

void lru_cache::release(int part_id, uint64_t bkt_idx) {
    get_latch();
    assert(_cache[part_id][bkt_idx] != nullptr);
    _cache[part_id][bkt_idx]->_read_cnt --;
    assert(_cache[part_id][bkt_idx]->_read_cnt >= 0);
    release_latch();
}

void *lru_cache::try_load(int part_id, uint64_t bkt_idx) {
    // use lock-free for a faster try cache. --> speed up from 84s to 16s.
    auto cur = _cache[part_id][bkt_idx];
    if (cur == nullptr) {
        return nullptr;
    }
    _cache[part_id][bkt_idx]->_read_cnt ++;
    _cache[part_id][bkt_idx]->_ts = ++ _timestamp;
    auto res = _cache[part_id][bkt_idx]->_value;
    return res;
}

void* lru_cache::free() {
    assert(false);
    while (!_history.empty()) {
        auto it = _history.front();
        auto cur = _cache[it.part][it.bkt];
        if (cur == nullptr || cur->_ts != it.ts) {
            // lazy delete of history.
            _history.pop();
        } else {
            _cache[it.part][it.bkt] = nullptr;
            auto res = cur->_value;
            _history.pop();
            delete cur;
            return res;
        }
    }
    return nullptr;
}

// swap and load;
void *lru_cache::load_and_swap(int part_id, uint64_t bkt_idx,  void *inserted, void* &swapped) {
    void * res;
    get_latch();
    swapped = nullptr;
    if (_cache[part_id][bkt_idx] != nullptr) {
        // loaded by others.
        res = _cache[part_id][bkt_idx]->_value;
        _cache[part_id][bkt_idx]->_read_cnt ++;
        _cache[part_id][bkt_idx]->_ts = ++ _timestamp;
    } else {
        res = inserted;
        if (_cached_cnt == _limit) {
            swapped = free();
            if (swapped != nullptr) {
                _cache[part_id][bkt_idx] = new cache_node(_timestamp, inserted);
            }
        } else {
            _cached_cnt ++;
            _cache[part_id][bkt_idx] = new cache_node(_timestamp, inserted);
        }
    }
    release_latch();
    return res;
}