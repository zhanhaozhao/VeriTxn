//
// Created by pan on 2023/1/13.
//

#ifndef DBX1000_HASH_CHAIN_H
#define DBX1000_HASH_CHAIN_H

//#include "global_enc.h"
//#include "mem_helper_enc.h"

struct hash_item {
    uint64_t value;
    uint64_t commit_ts;
    hash_item* next;
    hash_item(uint64_t ts, uint64_t val) {
        value = val;
        commit_ts = ts;
        next = nullptr;
    }
};

class hash_chain {
public:
    uint32_t size;
    hash_item* head, *tail;
    uint64_t batch_cnt;
    bool latch;

    hash_chain() {
        size = 0;
        head = tail = nullptr;
        batch_cnt = 0;
        latch = false;
    }

    void lock() {while (!ATOM_CAS(latch, false, true));}
    void unlock() {assert( ATOM_CAS(latch, true, false) );}

    bool empty() {
        lock();
        bool res = size == 0;
        unlock();
        return res;
    }

    void vaccum(uint64_t valid_ts) {
        lock();
        while (size > 1 && head->next && head->next->commit_ts <= valid_ts) {
            auto old_node = head;
            head = head -> next;
            delete old_node;
            size --;
        }
        unlock();
    }

    void insert(uint64_t ts, uint64_t value) {
        lock();
        size ++;
        auto new_node = new hash_item(ts, value);
        if (head == nullptr) {
            head = tail = new hash_item(ts, value);
            unlock();
        } else if (ts > tail->commit_ts) {
            tail -> next = new_node;
            tail = new_node;
            unlock();
        } else {
            if (ts < head->commit_ts) {
                new_node -> next = head;
                head = new_node;
                unlock();
                return;
            }
            auto last = head;
            for (auto i = head; i; i = i->next) {
                if (ts == i->commit_ts) {
                    assert(value == i->value);
                    size --;
                    unlock();
                    return;
                }
                if (i->commit_ts > ts) {
                    auto new_node = new hash_item(ts, value);
                    new_node->next = last->next;
                    last->next = new_node;
                    unlock();
                    return;
                }
                last = i;
            }
            unlock();
            assert(false);
        }
    }

    uint64_t get_max_ts() {
        lock();
        if (size == 0) {
            unlock();
            return 0;
        }
        auto res = tail->commit_ts;
        unlock();
        return res;
    }

    uint64_t get(uint64_t ts, uint64_t &length, uint64_t &rts) {
        lock();
        if (size == 0) {
            unlock();
            return -1;
        }
        assert(size > 0);
        length = size;
        uint64_t res = 0;
#if FAST_VERI_CHAIN_ACCESS == 1
        if (ts >= tail->commit_ts) {
            rts = tail->commit_ts;
            res = tail->value;
            unlock();
            return res;
        }
#endif
        auto last = head;
        if (ts < head->commit_ts) { // not loaded yet.
            unlock();
            return -1;
        }
//        assert(ts >= head->commit_ts);
        for (auto i = head; i; i=i->next) {
            if (ts < i->commit_ts) {
                rts = last->commit_ts;
                res = last->value;
                unlock();
                return res;
            }
            last = i;
        }
        rts = tail->commit_ts;
        res = tail->value;
        unlock();
        return res;
    }
};


#endif //DBX1000_HASH_CHAIN_H
