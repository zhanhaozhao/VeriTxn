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
    hash_item* head, *tail;
    uint32_t size;
public:
    hash_chain() {
        size = 0;
        head = tail = nullptr;
    }

    bool empty() {
        return size == 0;
    }

    void insert(uint64_t ts, uint64_t value) {
        size ++;
        if (head == nullptr) {
            head = tail = new hash_item(ts, value);
            size = 1;
        } else if (ts > tail->commit_ts) {
            tail -> next = new hash_item(ts, value);
        } else {
            if (ts < head->commit_ts) {
                auto new_node = new hash_item(ts, value);
                new_node -> next = head;
                head = new_node;
                return;
            }
            auto last = head;
            for (auto i = head; i; i = i->next) {
                if (ts == i->commit_ts) {
                    assert(value == i->value);
                    size --;
                    return;
                }
                if (i->commit_ts > ts) {
                    auto new_node = new hash_item(ts, value);
                    new_node->next = last->next;
                    last->next = new_node;
                    return;
                }
                last = i;
            }
            assert(false);
        }
    }

    uint64_t get_max_ts() {
        assert(size > 0);
        return tail->commit_ts;
    }

    uint64_t get(uint64_t ts, uint64_t &length) {
        assert(size > 0);
        length = size;
#if FAST_VERI_CHAIN_ACCESS == 1
        if (ts >= tail->commit_ts) return tail->value;
#endif
        auto last = head;
        assert(ts >= head->commit_ts);
        for (auto i = head; i; i=i->next) {
            if (ts < i->commit_ts) {
                return last->value;
            }
            last = i;
        }
        return tail->value;
    }
};


#endif //DBX1000_HASH_CHAIN_H
