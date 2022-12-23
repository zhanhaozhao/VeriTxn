// #include <mm_malloc.h>
#include "global.h"
#include "table.h"
#include "catalog.h"
#include "base_row.h"
#include "cstring"
#include "global_struct.h"
#include "coder.h"
// #include "txn.h"
// #include "row_lock.h"
// #include "row_ts.h"
// #include "row_mvcc.h"
// #include "row_hekaton.h"
// #include "row_occ.h"
// #include "row_tictoc.h"
// #include "row_silo.h"
// #include "row_vll.h"
// #include "common/mem_alloc.h"
// #include "manager.h"

//RC
//base_row_t::init(table_t * host_table, uint64_t part_id, uint64_t row_id) {
//	_row_id = row_id;
//	_part_id = part_id;
//	this->table = host_table;
//	Catalog * schema = host_table->get_schema();
//	int tuple_size = schema->get_tuple_size();
//	data = (char *) _mm_malloc(sizeof(char) * tuple_size, 64);
//	return RCOK;
//}
void 
base_row_t::init(int size) 
{
	data = (char *) _mm_malloc(size, 64);
}

RC 
base_row_t::switch_schema(table_t * host_table) {
	this->table = host_table;
	return RCOK;
}

void base_row_t::init_manager(base_row_t * row) {
#if CC_ALG == DL_DETECT || CC_ALG == NO_WAIT || CC_ALG == WAIT_DIE
    // manager = (Row_lock *) mem_allocator.alloc(sizeof(Row_lock), _part_id);
#elif CC_ALG == TIMESTAMP
    manager = (Row_ts *) mem_allocator.alloc(sizeof(Row_ts), _part_id);
#elif CC_ALG == MVCC
    manager = (Row_mvcc *) _mm_malloc(sizeof(Row_mvcc), 64);
#elif CC_ALG == HEKATON
    manager = (Row_hekaton *) _mm_malloc(sizeof(Row_hekaton), 64);
#elif CC_ALG == OCC
    // manager = (Row_occ *) mem_allocator.alloc(sizeof(Row_occ), _part_id);
#elif CC_ALG == TICTOC
//	manager = (Row_tictoc *) _mm_malloc(sizeof(Row_tictoc), 64);
#elif CC_ALG == SILO
	manager = (Row_silo *) _mm_malloc(sizeof(Row_silo), 64);
#endif

#if CC_ALG != HSTORE
	// manager->init(this);
#endif
}

table_t * base_row_t::get_table() { 
	return table; 
}

Catalog * base_row_t::get_schema() { 
	return get_table()->get_schema(); 
}

const char* base_row_t::get_table_name() {
	return get_table()->get_table_name(); 
};
//uint64_t base_row_t::get_tuple_size() {
//	return table->get_schema()->get_tuple_size();
//}

uint64_t base_row_t::get_field_cnt() { 
	return get_schema()->field_cnt;
}

void base_row_t::set_value(int id, void * ptr) {
	int datasize = get_schema()->get_field_size(id);
	int pos = get_schema()->get_field_index(id);
	memcpy( &data[pos], ptr, datasize);
}

void base_row_t::set_value(int id, void * ptr, int size) {
	int pos = get_schema()->get_field_index(id);
	memcpy( &data[pos], ptr, size);
}

void base_row_t::set_value(const char * col_name, void * ptr) {
	uint64_t id = get_schema()->get_field_id(col_name);
	set_value(id, ptr);
}

SET_VALUE_BASE(uint64_t);
SET_VALUE_BASE(int64_t);
SET_VALUE_BASE(double);
SET_VALUE_BASE(UInt32);
SET_VALUE_BASE(SInt32);

GET_VALUE_BASE(uint64_t);
GET_VALUE_BASE(int64_t);
GET_VALUE_BASE(double);
GET_VALUE_BASE(UInt32);
GET_VALUE_BASE(SInt32);

char * base_row_t::get_value(int id) {
	int pos = get_schema()->get_field_index(id);
	return &data[pos];
}

char * base_row_t::get_value(char * col_name) {
	uint64_t pos = get_schema()->get_field_index(col_name);
	return &data[pos];
}

char * base_row_t::get_data() { return data; }

void base_row_t::set_data(char * data, uint64_t size) { 
	memcpy(this->data, data, size);
}
// copy from the src to this
void base_row_t::copy(base_row_t * src) {
	set_data(src->get_data(), src->get_tuple_size());
}

//void base_row_t::set_row_id(uint64_t id) {
//    _row_id = id;
//}

void base_row_t::free_row() {
	free(data);
}

// #include "string"
// #include "vector"
// #include "coder.h"

std::string base_row_t::encode() {
    std::vector<std::pair<std::string, std::string> > data_items;
//    puts(table->get_table_name());
    data_items.emplace_back(make_pair("table_name:", std::string(table->get_table_name())));
    data_items.emplace_back(make_pair("primary_key:", std::to_string(_primary_key)));
    data_items.emplace_back(make_pair("part_id:", std::to_string(_part_id)));
    data_items.emplace_back(make_pair("row_id:", std::to_string(_row_id)));
    auto siz = get_tuple_size();
    std::string data_str;
    for (unsigned i=0;i<siz;i++) {
        uint8_t val = data[i];  // 256.
        data_str.push_back(val % 10 + '0');
        data_str.push_back((val/10) % 10 + '0');
        data_str.push_back((val/100) % 10 + '0');
    }
    data_items.emplace_back(std::make_pair("data:", data_str));
    return encode_vec(data_items);
}
//
//std::uint64_t base_row_t::hash() {
//    uint64_t res = 0;
//    res ^= _primary_key ^ _row_id;
//    auto siz = get_tuple_size();
//    for (unsigned i=0;i<siz;i++) {
//        auto val = (unsigned char) (data[i]);  // 256.
//        res ^= uint64_t (val);
//    }
//    return res;
//}

void base_row_t::decode(const std::string& e) {
    std::vector<std::pair<std::string, std::string> > data_items = decode_vec(e);
    assert(data_items.size() == 5);
    const std::string &data_s = data_items[4].second;
    this->init(int(data_s.length()));
//    auto nam = data_items[0].second.c_str();
    this->table = (table_t*)global_table_map->_tables[data_items[0].second];
    assert(this->table);
    memcpy(data, data_s.c_str(), data_s.length());
    for (size_t i=0;i<data_s.length();i+=3) {
        data[i] = data_s[i] - '0';
        data[i] += (data_s[i+1] - '0') * 10;
        data[i] += (data_s[i+2] - '0') * 100;
    }
    set_primary_key(int64_t(stoull(data_items[1].second)));
    _part_id = (int64_t(stoull(data_items[2].second)));
    _row_id = (int64_t(stoull(data_items[3].second)));
    init_manager(this);
}
