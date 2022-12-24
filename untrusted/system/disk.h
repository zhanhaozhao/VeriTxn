#include "common/config.h"
#include "common/index_hash.h"
#include "common/index_btree.h"

// storage the value of c to key <iname, part_id, bkt_idx>
void flush_out_disk(std::string iname, int part_id, uint64_t pg_id, PAGE *c) {
}

PAGE* load_page_disk(std::string iname, int part_id, uint64_t pg_id) {
    return nullptr;
}