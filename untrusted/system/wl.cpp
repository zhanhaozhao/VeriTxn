#include <vector>
#include <fstream>

#include "global.h"
#include "global_struct.h"
// #include "common/helper.h"

#include "wl.h"
#include "base_row.h"
#include "table.h"
#include "index_hash.h"
#include "index_btree.h"
// #include "catalog.h"
// #include "common/mem_alloc.h"
#include "thread_enc.h"
#include "table_map.h"

RC workload::init() {
	sim_done = false;
	return RCOK;
}

RC workload::init_schema(std::string schema_file) {
//    global_table_map = new table_map;
    assert(sizeof(uint64_t) == 8);
    assert(sizeof(double) == 8);	
	std::string line;
	std::ifstream fin(schema_file);
    Catalog * schema;
    while (getline(fin, line)) {
		if (line.compare(0, 6, "TABLE=") == 0) {
			std::string tname;
			tname = &line[6];
			schema = (Catalog *) _mm_malloc(sizeof(Catalog), CL_SIZE);
			getline(fin, line);
			int col_count = 0;
			// Read all fields for this table.
			std::vector<std::string> lines;
			while (line.length() > 1) {
				lines.push_back(line);
				getline(fin, line);
			}
			schema->init( tname.c_str(), lines.size() );
			for (UInt32 i = 0; i < lines.size(); i++) {
				std::string line = lines[i];
			    size_t pos = 0;
				std::string token;
				int elem_num = 0;
				int size = 0;
				std::string type;
				std::string name;
				while (line.length() != 0) {
					pos = line.find(",");
					if (pos == std::string::npos)
						pos = line.length();
	    			token = line.substr(0, pos);
			    	line.erase(0, pos + 1);
					switch (elem_num) {
					case 0: size = atoi(token.c_str()); break;
					case 1: type = token; break;
					case 2: name = token; break;
					default: assert(false);
					}
					elem_num ++;
				}
				assert(elem_num == 3);
                schema->add_col((char *)name.c_str(), size, (char *)type.c_str());
				col_count ++;
			}
			table_t * cur_tab = (table_t *) _mm_malloc(sizeof(table_t), CL_SIZE);
			cur_tab->init(schema);
			int n = tname.length();
            cur_tab->table_name = new char[n+1];
            tname.copy(cur_tab->table_name, n);
            cur_tab->table_name[n] = 0;
			tables[tname] = cur_tab;
            assert(std::string(tables[tname]->get_table_name()) == tname);
			global_table_map->_tables[tname] = cur_tab;
        } else if (!line.compare(0, 6, "INDEX=")) {
			std::string iname;
			iname = &line[6];
			getline(fin, line);

			std::vector<std::string> items;
			std::string token;
			size_t pos;
			while (line.length() != 0) {
				pos = line.find(",");
				if (pos == std::string::npos)
					pos = line.length();
	    		token = line.substr(0, pos);
				items.push_back(token);
		    	line.erase(0, pos + 1);
			}
			
			std::string tname(items[0]);
			INDEX * index = (INDEX *) _mm_malloc(sizeof(INDEX), 64);
			new(index) INDEX();
			int part_cnt = (CENTRAL_INDEX)? 1 : g_part_cnt;
			if (tname == "ITEM")
				part_cnt = 1;
#if INDEX_STRUCT == IDX_HASH
	#if WORKLOAD == YCSB
			index->init(part_cnt, tables[tname], (g_synth_table_size * 2) / BUCKET_FACTOR);
			int n = iname.length();
            index->index_name = new char[n+1];
			iname.copy(index->index_name, n);
			index->index_name[n] = 0;
			index_init_ecall(part_cnt, (void *) tables[tname], iname, (void*) index, (g_synth_table_size * 2) / BUCKET_FACTOR);
			global_table_map->_indexes[iname] = index;
	#elif WORKLOAD == TPCC
			assert(tables[tname] != NULL);
            auto size = stoull( items[1] ) * part_cnt * 10;
			index->init(part_cnt, tables[tname], size);
			int n = iname.length();
            index->index_name = new char[n+1];
			iname.copy(index->index_name, n);
			index->index_name[n] = 0;
            assert(std::string(index->index_name) == iname);
            index_init_ecall(part_cnt, (void *) (tables[tname]), iname, (void*)index , size / BUCKET_FACTOR);
//            printf("%s from %s\n", iname.c_str(), tname.c_str());
            global_table_map->_indexes[iname] = index;
	#endif
#else
			index->init(part_cnt, tables[tname]);
            global_table_map->_indexes[iname] = index;
#endif
			indexes[iname] = index;
			index->index_name = (char*) malloc(sizeof (char) * (iname.length()+1));
            index->index_name[iname.length()] = 0;
			memcpy(index->index_name, iname.c_str(), iname.length());
		}
    }
	fin.close();
	return RCOK;
}



void workload::index_insert(std::string index_name, uint64_t key, base_row_t * row) {
	assert(false);
	INDEX * index = (INDEX *) indexes[index_name];
	index_insert(index, key, row);
}

void workload::index_insert(INDEX * index, uint64_t key, base_row_t * row, int64_t part_id) {
	uint64_t pid = part_id;
	if (part_id == -1)
		pid = get_part_id(row);
	itemid_t * m_item =
		(itemid_t *) mem_allocator.alloc( sizeof(itemid_t), pid );
	m_item->init();
	m_item->type = DT_row;
	m_item->location = row;
	m_item->valid = true;
	m_item->next = nullptr;

    assert( index->index_insert(key, m_item, pid) == RCOK );
}


