#include <coder.h>
#include "global.h"
#include "global_struct.h"

#include "index_hash.h"

// #include "common/mem_alloc.h"
#include "table.h"
#include "disk.h"

RC IndexHash::init(uint64_t bucket_cnt, int part_cnt) {
//	_bucket_cnt = bucket_cnt;
//	_part_cnt = part_cnt;
	_bucket_cnt_per_part = bucket_cnt / part_cnt;
	_buckets = new BucketHeader * [part_cnt];
	for (int i = 0; i < part_cnt; i++) {
		_buckets[i] = (BucketHeader *) _mm_malloc(sizeof(BucketHeader) * _bucket_cnt_per_part, 64);
		for (uint32_t n = 0; n < _bucket_cnt_per_part; n ++)
			_buckets[i][n].init();
	}
	return RCOK;
}

RC 
IndexHash::init(int part_cnt, table_t * table, uint64_t bucket_cnt) {
	init(bucket_cnt, part_cnt);
	this->table = table;
	return RCOK;
}

bool IndexHash::index_exist(idx_key_t key) {
	assert(false);
}

//void
//IndexHash::get_latch(BucketHeader * bucket) {
//	while (!ATOM_CAS(bucket->locked, false, true)) {}
//}
//
//void
//IndexHash::release_latch(BucketHeader * bucket) {
//	bool ok = ATOM_CAS(bucket->locked, true, false);
//	assert(ok);
//}

	
RC IndexHash::index_insert(idx_key_t key, itemid_t * item, int part_id) {
	RC rc = RCOK;
	uint64_t bkt_idx = hash(key);
	assert(bkt_idx < _bucket_cnt_per_part);
	BucketHeader * cur_bkt = &_buckets[part_id][bkt_idx];
	// 1. get the ex latch
	get_latch(cur_bkt);
	
	// 2. update the latch list
	cur_bkt->insert_item(key, item, part_id);
//    if (key == 1) {
//        printf("After insert (%s) %s\n", ((base_row_t*)item->location)->encode().c_str(), cur_bkt->encode().c_str());
//        printf("After insert (%s) (%s) (%s)\n", cur_bkt->encode().c_str(), cur_bkt->first_node->encode().c_str(),
//               ((base_row_t*)cur_bkt->first_node->items->location)->encode().c_str());
//        assert(false);
//    }

	// 3. release the latch
	release_latch(cur_bkt);
	return rc;
}

RC IndexHash::index_read(idx_key_t key, itemid_t * &item, int part_id) {
	uint64_t bkt_idx = hash(key);
	assert(bkt_idx < _bucket_cnt_per_part);
	BucketHeader * cur_bkt = &_buckets[part_id][bkt_idx];
	RC rc = RCOK;
	// 1. get the sh latch
//	get_latch(cur_bkt);
	cur_bkt->read_item(key, item, std::string(table->get_table_name()));
	// 3. release the latch
//	release_latch(cur_bkt);
	return rc;

}

RC IndexHash::index_read(idx_key_t key, itemid_t * &item, 
						int part_id, int thd_id) {
	uint64_t bkt_idx = hash(key);
	assert(bkt_idx < _bucket_cnt_per_part);
	BucketHeader * cur_bkt = &_buckets[part_id][bkt_idx];
	RC rc = RCOK;
	// 1. get the sh latch
//	get_latch(cur_bkt);
	cur_bkt->read_item(key, item, std::string(table->get_table_name()));
	// 3. release the latch
//	release_latch(cur_bkt);
	return rc;
}

//uint64_t BucketHeader::get_hash() const {
//    uint64_t res = 0;
//    BucketNode * cur_node = first_node;
//    while (cur_node != nullptr) {
//        res ^= cur_node->hash() ^ cur_node->key;
//        cur_node = cur_node->next;
//    }
//    return res;
//}

//uint64_t BucketNode::hash() const {
//    uint64_t res = 0;
//    itemid_t * it = items;
//    res ^= key;
//    while (it != nullptr) {
//        auto tmp = (base_row_t*)(it->location);
//        res ^= tmp->hash();
//        it = it -> next;
//    }
//    return res;
//}

BucketHeader *	IndexHash::load_bucket(int part_id, int bkt_idx) {
	BucketHeader * cur_bkt = &_buckets[part_id][bkt_idx];
	return cur_bkt;
}

// RC * IndexHash::sync_bucket_from_disk(int part_id, int bkt_idx) {
    // Since current tables are kept in memory and not flushed into disk during init_table.
    // We call a load disk to simulate one layer cache.
    // remotestorage->load_page_disk(index_name, part_id, bkt_idx);
// }

/************** BucketHeader Operations ******************/

//void BucketHeader::init() {
//	node_cnt = 0;
//	first_node = NULL;
//	locked = false;
//}

void BucketHeader::insert_item(idx_key_t key, 
		itemid_t * item, 
		int part_id) 
{
	BucketNode * cur_node = first_node;
	BucketNode * prev_node = NULL;
	while (cur_node != NULL) {
		if (cur_node->key == key)
			break;
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	if (cur_node == NULL) {		
		BucketNode * new_node = (BucketNode *)
			mem_allocator.alloc(sizeof(BucketNode), part_id );
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

void BucketHeader::read_item(idx_key_t key, itemid_t * &item, std::string tname)
{
	BucketNode * cur_node = first_node;
	while (cur_node != NULL) {
		if (cur_node->key == key)
			break;
		cur_node = cur_node->next;
	}
	// M_ASSERT(cur_node->key == key, "Key does not exist!");
	assert(cur_node->key == key);
	item = cur_node->items;
}

DFlow BucketHeader::encode() const {
    std::vector <encoded_record> data;
    BucketNode * cur_node = first_node;
    while (cur_node != nullptr) {
        data.emplace_back(make_pair(std::to_string(cur_node->key), cur_node->encode()));
        cur_node = cur_node->next;
    }
    return encode_vec(data);
}

void BucketHeader::decode(const DFlow & e) {
    std::vector <encoded_record> data = decode_vec(e);
    this->init();
    BucketNode* last = nullptr;
    for (const auto& it:data) {
        auto tmp = new BucketNode(stoull(it.first));
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

DFlow BucketNode::encode() const {
    std::vector <encoded_record> data;
    std::string res_items;
    itemid_t * it = items;
    data.emplace_back(std::make_pair(std::to_string(this->key), ""));
    while (it != nullptr) {
        auto tmp = (base_row_t*)(it->location);
        data.emplace_back(std::make_pair(std::to_string(tmp->get_part_id()), tmp->encode()));
        it = it -> next;
    }
    return encode_vec(data);
}

void BucketNode::decode(const DFlow & e) {
    std::vector <encoded_record> data = decode_vec(e);
    this->init(std::stoull(data[0].first));
    this->items = new itemid_t;
    this->items->init();
    int n = data.size();
    itemid_t* last = nullptr;
    for (int i = 1;i < n;i ++) {
        auto * cur_row = new base_row_t;
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