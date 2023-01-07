#ifndef TABLE_H_
#define TABLE_H_

// #include "global_common.h"
#include "catalog.h"
// #include "row.h"

// TODO sequential scan is not supported yet.
// only index access is supported for table. 
// class Catalog;
class base_row_t;

class table_t
{
public:
    table_t(){cur_tab_size = 0;}
	void init(Catalog * catalog);
	// row lookup should be done with index. But index does not have
	// records for new rows. get_new_row returns the pointer to a 
	// new row.	
	RC get_new_row(base_row_t *& row); // this is equivalent to insert()
	RC get_new_row(base_row_t *& row, uint64_t part_id, uint64_t &row_id);

	void delete_row(); // TODO delete_row is not supportet yet

	uint64_t get_table_size() { return cur_tab_size; };
	Catalog * get_schema() { return schema; };
	const char* get_table_name() { return table_name; };

    char* 	table_name;
    uint64_t  		cur_tab_size;
	Catalog * 		schema;
    char 			pad[CL_SIZE * 2];
};

#endif