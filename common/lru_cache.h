#include <atomic>
#include "global_common.h"
#include "helper.h"
#include "queue"

#ifndef DBX1000_LRU_CACHE_H
#define DBX1000_LRU_CACHE_H

struct cache_node;
struct cache_visit;

// TODO: support _cache lease for read only node.
class lru_cache {
public:
    RC 	init(uint64_t bucket_cnt, int part_cnt, uint64_t siz);
    void *try_load(int part_id, uint64_t bkt_idx);
    void *load_and_swap(int part_id, uint64_t bkt_idx,  void *inserted, void* &swapped, int &swapped_part, uint64_t &swapped_idx);
    void release(int part_id, uint64_t bkt_idx);
    void inc_lease(int part_id, uint64_t bkt_idx);
    void reset_lease(int part_id, uint64_t bkt_idx);

private:
    bool locked{};
    cache_node*** _cache{};
    uint64_t** _lease;
    int n;
    uint64_t m;
    std::queue<cache_visit> _history;
    uint64_t _cached_cnt{};
    uint64_t _timestamp{};
    uint64_t _limit{};
    void *cache_free(int &part, uint64_t & bkt);
    void get_latch() {
        while (!ATOM_CAS(this->locked, false, true)) {}
    };
    void release_latch() {
        bool ok = ATOM_CAS(this->locked, true, false);
        assert(ok);
    };
};

struct cache_visit {
    uint64_t bkt;
    int part;
    uint64_t ts;
};

struct cache_node {
    uint64_t _ts;
    uint64_t _read_cnt;
    void* _value;

    cache_node(uint64_t ts, void* value) {
        _read_cnt = 1;
        _ts = ts;
        _value = value;
    }
};

#endif //DBX1000_LRU_CACHE_H
