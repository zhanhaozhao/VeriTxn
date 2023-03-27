#include <sched.h>
#include <thread_enc.h>
#include "global.h"
#include "global_struct.h"
// #include "common/helper.h"
#include "ycsb.h"
#include "wl.h"
// #include "common/thread.h"
#include "table.h"
#include "base_row.h"
#include "index_hash.h"
#include "index_btree.h"
#include "thread_enc.h"
// #include "catalog.h"
// #include "manager.h"
// #include "row_lock.h"
// #include "row_ts.h"
// #include "row_mvcc.h"
// #include "common/mem_alloc.h"
// #include "common/query.h"
// #include "ycsb_txn.h"

int ycsb_wl::next_tid;

RC ycsb_wl::init() {
	workload::init();
	next_tid = 0;
	printf("begin to init [YCSB] Table.\n");
	std::string path = "./untrusted/benchmarks/YCSB_schema.txt";
	init_schema( path );

    init_table();
	int part_cnt = (CENTRAL_INDEX)? 1 : g_part_cnt;
#if VERI_TYPE == MERKLE_TREE
    the_index->calculate_hash();
#endif
	index_load_ecall(part_cnt, (void *) (the_table), "MAIN_INDEX", (void*)the_index , g_synth_table_size * 2);
    return RCOK;
}

RC ycsb_wl::init_schema(std::string schema_file) {
	workload::init_schema(schema_file);
	the_table = tables["MAIN_TABLE"]; 	
	the_index = indexes["MAIN_INDEX"];
	return RCOK;
}
	
// int 
// ycsb_wl::key_to_part(uint64_t key) {
// 	uint64_t rows_per_part = g_synth_table_size / g_part_cnt;
// 	return key / rows_per_part;
// }

RC ycsb_wl::init_table() {
	RC rc;
    uint64_t total_row = 0;
    while (true) {
    	for (UInt32 part_id = 0; part_id < g_part_cnt; part_id ++) {
            if (total_row > g_synth_table_size)
                goto ins_done;
            base_row_t * new_row = NULL;
			uint64_t row_id;
            rc = the_table->get_new_row(new_row, part_id, row_id); 
            // TODO insertion of last row may fail after the table_size
            // is updated. So never access the last record in a table
			assert(rc == RCOK);
			uint64_t primary_key = total_row;
			new_row->set_primary_key(primary_key);
            new_row->set_value(0, &primary_key);
            Catalog * schema = the_table->get_schema();
            for (UInt32 fid = 0; fid < schema->get_field_cnt(); fid ++) {
                char value[6] = "hello";
                new_row->set_value(fid, value);
            }

            itemid_t * m_item =
                    (itemid_t *) mem_allocator.alloc( sizeof(itemid_t), part_id );
            assert(m_item != NULL);
            m_item->type = DT_row;
            m_item->location = new_row;
            m_item->valid = true;
            m_item->next = nullptr;
            uint64_t idx_key = primary_key;
            rc = the_index->index_insert(idx_key, m_item, part_id);
            assert(rc == RCOK);
            total_row ++;
        }
    }
ins_done:
    index_init_ecall(1, (void *) tables["MAIN_TABLE"], "MAIN_INDEX", (void*) the_index, (g_synth_table_size * 2) / BUCKET_FACTOR);
//    printf("[YCSB] Table \"MAIN_TABLE\" initialized.\n");
    return RCOK;

}

// init table in parallel
void ycsb_wl::init_table_parallel() {
	enable_thread_mem_pool = true;
	pthread_t p_thds[g_init_parallelism - 1];
	for (UInt32 i = 0; i < g_init_parallelism - 1; i++) 
		pthread_create(&p_thds[i], NULL, threadInitTable, this);
	threadInitTable(this);

	for (uint32_t i = 0; i < g_init_parallelism - 1; i++) {
		int rc = pthread_join(p_thds[i], NULL);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}
	enable_thread_mem_pool = false;
	mem_allocator.unregister();
    index_init_ecall(1, (void *) tables["MAIN_TABLE"], "MAIN_INDEX", (void*) the_index, (g_synth_table_size * 2) / BUCKET_FACTOR);
}

void * ycsb_wl::init_table_slice() {
	UInt32 tid = ATOM_FETCH_ADD(next_tid, 1);
	// set cpu affinity
	set_affinity(tid);

	mem_allocator.register_thread(tid);
	RC rc;
	uint64_t key_cnt = 0;
	assert(g_synth_table_size % g_init_parallelism == 0);
	assert(tid < g_init_parallelism);
	while ((UInt32)ATOM_FETCH_ADD(next_tid, 0) < g_init_parallelism) {}
	assert((UInt32)ATOM_FETCH_ADD(next_tid, 0) == g_init_parallelism);
	uint64_t slice_size = g_synth_table_size / g_init_parallelism;
//    printf("up to %lu %u\n", slice_size * (tid + 1), tid);
    uint64_t max_key = slice_size * (tid + 1);
    if ((tid+1) == g_init_parallelism) {
        max_key = g_synth_table_size;
    }
	for (uint64_t key = slice_size * tid; 
			key < max_key;
			key ++
	) {
		base_row_t * new_row = NULL;
		uint64_t row_id;
		int part_id = key_to_part(key);
		rc = the_table->get_new_row(new_row, part_id, row_id); 
		assert(rc == RCOK);
		uint64_t primary_key = key;
		new_row->set_primary_key(primary_key);
		new_row->set_value(0, &primary_key);
		Catalog * schema = the_table->get_schema();
		
		for (UInt32 fid = 0; fid < schema->get_field_cnt(); fid ++) {
			char value[6] = "hello";
			new_row->set_value(fid, value);
		}

		itemid_t * m_item =
			(itemid_t *) mem_allocator.alloc( sizeof(itemid_t), part_id );
		assert(m_item != NULL);
		m_item->type = DT_row;
		m_item->location = new_row;
		m_item->valid = true;
		m_item->next = nullptr;
		uint64_t idx_key = primary_key;
		
		rc = the_index->index_insert(idx_key, m_item, part_id);
		assert(rc == RCOK);
	}
	return NULL;
}

// RC ycsb_wl::get_txn_man(txn_man *& txn_manager, thread_t * h_thd){
// 	txn_manager = (ycsb_txn_man *)
// 		_mm_malloc( sizeof(ycsb_txn_man), 64 );
// 	new(txn_manager) ycsb_txn_man();
// 	txn_manager->init(h_thd, this, h_thd->get_thd_id());
// 	return RCOK;
// }

