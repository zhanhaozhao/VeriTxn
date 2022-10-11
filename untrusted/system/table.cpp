#include "global.h"
#include "common/helper.h"
#include "table.h"

#include <utility>
#include "catalog.h"
#include "base_row.h"
#include "mem_alloc.h"

void table_t::init(Catalog * catalog) {
    this->schema = catalog;
	assert(this->schema->field_cnt);
}

RC table_t::get_new_row(base_row_t *& row) {
	// this function is obsolete. 
	assert(false);
	return RCOK;
}

// the row is not stored locally. the pointer must be maintained by index structure.
RC table_t::get_new_row(base_row_t *& row, uint64_t part_id, uint64_t &row_id) {
	RC rc = RCOK;
	cur_tab_size ++;
	row_id = random() & 0xFFFFFFF;
	
	row = (base_row_t *) _mm_malloc(sizeof(base_row_t), 64);
	rc = row->init(this, part_id, row_id);
	row->init_manager(row);

	return rc;
}
