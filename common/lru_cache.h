#include <atomic>
#include "global_common.h"
#include "helper.h"
#include "queue"

#ifndef DBX1000_LRU_CACHE_H
#define DBX1000_LRU_CACHE_H

struct cache_node;
struct cache_visit;

struct cache_visit {
    uint64_t bkt;
    int part;
    uint64_t ts;
    uint _size;
};

// TODO: support _cache lease for read only node.
class lru_cache {
public:
    RC 	init(uint64_t bucket_cnt, int part_cnt, uint64_t siz);
    void *try_load(uint part_id, uint64_t bkt_idx);
    void * release(int part_id, uint64_t bkt_idx);
    bool can_force_load() const; // if half of the place is occupied, then stop force loading.
    void inc_lease(int part_id, uint64_t bkt_idx);
    void reset_lease(int part_id, uint64_t bkt_idx);

    RC load_and_swap(int part_id, uint64_t bkt_idx, uint bytes_size, void *inserted, void *&swapped, int &swapped_part,
                      uint64_t &swapped_idx, void *&res);

private:
    bool locked{};
    cache_node*** _cache{};
    uint64_t** _lease;
    int n;
    uint64_t m;
#if LAZY_OFFLOADING == 1
    std::queue<cache_visit> free_pool;
    RC cache_free(int &part, uint64_t &bkt, void * &swapped);
#endif
    uint64_t _cached_bytes{};
    uint64_t _timestamp{};
    uint64_t _limit{};
    void get_latch() {
        while (!ATOM_CAS(this->locked, false, true)) {}
    };
    void release_latch() {
        bool ok = ATOM_CAS(this->locked, true, false);
        assert(ok);
    };

};

struct cache_node {
    uint64_t _ts;
    uint64_t _read_cnt;
    void* _value;
    uint _size;

    cache_node(uint64_t ts, void* value, uint size) {
        _read_cnt = 1;
        _ts = ts;
        _value = value;
        _size = size;
    }
};

#endif //DBX1000_LRU_CACHE_H
