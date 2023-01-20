#include "lru_cache.h"

#ifdef READ_ONLY
    const bool enable_lease = true;
#else
// cached data in RW node will not be modified by RO node.
    const bool enable_lease = false;
#endif

RC lru_cache::init(uint64_t bucket_cnt, int part_cnt, uint64_t siz) {
    locked = false;
    _cache = new cache_node** [part_cnt];
    _lease = new uint64_t* [part_cnt];
    _cached_bytes = 0;
    n = part_cnt;
    m = uint64_t (bucket_cnt/part_cnt);
    _limit = siz;
    for (int i = 0; i < part_cnt; i++) {
        _cache[i] = new cache_node* [bucket_cnt / part_cnt];
        _lease[i] = new uint64_t [bucket_cnt / part_cnt];
        for (uint32_t j = 0; j < bucket_cnt / part_cnt; j ++) {
            _lease[i][j] = BASE_LEASE;
            _cache[i][j] = nullptr;
        }
    }
    return RCOK;
}

void* lru_cache::release(int part_id, uint64_t bkt_idx) {
    get_latch();
    if (_cache[part_id][bkt_idx] == nullptr) {
        // the data is already released.
        release_latch();
        return nullptr;
    }
    _cache[part_id][bkt_idx]->_read_cnt  = std::max(_cache[part_id][bkt_idx]->_read_cnt-1, uint64_t(0));
    assert(_cache[part_id][bkt_idx]->_read_cnt >= 0);
    if (_cache[part_id][bkt_idx]->_read_cnt == 0) {
#if LAZY_OFFLOADING == 1
        free_pool.push(cache_visit{ bkt: bkt_idx, part: part_id,
                                    ts: _cache[part_id][bkt_idx]->_ts, _size:_cache[part_id][bkt_idx]->_size});
#else
        auto res = _cache[part_id][bkt_idx]->_value;
        delete _cache[part_id][bkt_idx];
        _cache[part_id][bkt_idx] = nullptr;
        release_latch();
        return res;
#endif
    }
    release_latch();
    return nullptr;
}

void *lru_cache::try_load(uint part_id, uint64_t bkt_idx) {
    // use lock-free for a faster try cache. --> speed up from 84s to 16s.
    assert(part_id >= 0 && part_id < n && bkt_idx >=0 && bkt_idx < m);
    auto cur = _cache[part_id][bkt_idx];
    if (cur == nullptr || (cur->_ts + _lease[part_id][bkt_idx] > _timestamp && enable_lease)) {
        return nullptr;
    }
    cur->_read_cnt++;
    cur->_ts = std::max(cur->_ts, ++ _timestamp);
    auto res = cur->_value;
    return res;
}

#if LAZY_OFFLOADING == 1
RC lru_cache::cache_free(int &part, uint64_t & bkt, void * &swapped) {
//#ifdef PRE_LOAD
//    assert(false);
//#endif
    while (!free_pool.empty()) {
        auto it = free_pool.front();
        auto cur = _cache[it.part][it.bkt];
        if (cur == nullptr || cur->_ts != it.ts) {
            // lazy delete of history.
            free_pool.pop();
        } else {
            _cache[it.part][it.bkt] = nullptr;
            auto res = cur->_value;
            part = it.part;
            bkt = it.bkt;
            _cached_bytes -= it._size;
            free_pool.pop();
            delete cur;
            swapped = res;
            return RCOK;
        }
    }
    return Abort;
}
#endif

void lru_cache::reset_lease(int part_id, uint64_t bkt_idx) {
    if (!enable_lease) {
        return;
    }
    get_latch();
    _lease[part_id][bkt_idx] = BASE_LEASE;
    release_latch();
}

void lru_cache::inc_lease(int part_id, uint64_t bkt_idx) {
    if (!enable_lease) {
        return;
    }
    get_latch();
    _lease[part_id][bkt_idx] *= 2;
    release_latch();
}

// swap and load;
RC lru_cache::load_and_swap(int part_id, uint64_t bkt_idx, uint bytes_size,  void *inserted, void* &swapped, int &swapped_part, uint64_t &swapped_idx, void * &res) {
    assert(part_id >= 0 && part_id < n && bkt_idx >=0 && bkt_idx < m);
    get_latch();
    swapped = nullptr, swapped_idx = 0, swapped_part = 0;
    if (_cache[part_id][bkt_idx] != nullptr && (!_cache[part_id][bkt_idx]->_ts + _lease[part_id][bkt_idx] > _timestamp || !enable_lease)) {
        // loaded by others.
        res = _cache[part_id][bkt_idx]->_value;
        _cache[part_id][bkt_idx]->_read_cnt ++;
        _cache[part_id][bkt_idx]->_ts = ++ _timestamp;
    } else if (_cache[part_id][bkt_idx] != nullptr) {
        swapped = _cache[part_id][bkt_idx]->_value;
        swapped_part = part_id, swapped_idx = bkt_idx;
        delete _cache[part_id][bkt_idx];
        _cache[part_id][bkt_idx] = new cache_node(++_timestamp, inserted, bytes_size);
        res = inserted;
    } else {
        res = inserted;
        if (_cached_bytes + bytes_size > _limit) {
#if LAZY_OFFLOADING == 1
            auto rc = cache_free(swapped_part, swapped_idx, swapped);
            if (rc == Abort) {
                release_latch();
                return Abort;
            }
            if (swapped != nullptr) {
                _cache[part_id][bkt_idx] = new cache_node(++_timestamp, inserted, bytes_size);
            }
#else
            release_latch();
            return Abort;
#endif
        } else {
            _cached_bytes += bytes_size;
            _cache[part_id][bkt_idx] = new cache_node(++_timestamp, inserted, bytes_size);
        }
    }
    release_latch();
    return RCOK;
}

bool lru_cache::can_force_load() const {
    return _cached_bytes * 2 <= _limit;
}
