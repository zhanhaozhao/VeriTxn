#include "disk.h"

void disk::init_disk(int part_cnt, uint64_t bucket_cnt) {
    _disk = new std::string * [part_cnt];
    for (int i = 0; i < part_cnt; i++) {
        uint64_t _bucket_cnt_per_part = bucket_cnt / part_cnt;
        _disk[i] = new std::string [_bucket_cnt_per_part];
        for (uint64_t j = 0; j < _bucket_cnt_per_part; j ++) {
            _disk[i][j] = "";
        }
    }
}

std::string disk::get_bucket_disk(int part_id, uint64_t bkt_idx) {
    return _disk[part_id][bkt_idx];
}

void disk::put_bucket_disk(int part_id, uint64_t bkt_idx, const std::string& e) {
    _disk[part_id][bkt_idx] = e;
}