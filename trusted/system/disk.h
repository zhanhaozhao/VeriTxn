//
// Created by pan on 2022/9/27.
//

#ifndef DBX1000_DISK_H
#define DBX1000_DISK_H

#include "string"
#include "global_common.h"


class disk {
    std::string ** _disk;
public:
    void init_disk(int part_cnt, uint64_t bucket_cnt);
    std::string get_bucket_disk(int part_id, uint64_t bkt_idx);
    void put_bucket_disk(int part_id, uint64_t bkt_idx, const std::string& e);
};


#endif //DBX1000_DISK_H
