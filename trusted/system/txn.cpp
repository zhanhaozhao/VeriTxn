// #include "global_enc.h"
#include "global_enc_struct.h"
#include "global_enc.h"
#include "index_btree_enc.h"

#include "txn.h"
#include "row_enc.h"
// #include "wl.h"
// #include "ycsb.h"
// #include "common/thread.h"
// #include "common/mem_alloc.h"
#include "occ.h"
#include "table.h"
#include "catalog.h"
#include "mem_helper_enc.h"
#include "index_btree.h"
#include "index_hash.h"
#include "index_enc.h"
#include "logger_enc.h"
#include "helper.h"

#include "api.h"

void myrand_enc::init(uint64_t seed) {
    this->seed = seed;
}

uint64_t myrand_enc::next() {
    seed = (seed * 1103515247UL + 12345UL) % (1UL<<63);
    return (seed / 65537) % RAND_MAX;
}

void txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
	this->h_thd = h_thd;
	this->h_wl = h_wl;
	mrand = new myrand_enc;
	mrand->init(thd_id ^ 21312);
	pthread_mutex_init(&txn_lock, NULL);
	lock_ready = false;
	ready_part = 0;
	row_cnt = 0;
	wr_cnt = 0;
	insert_cnt = 0;
	// accesses = (Access **) aligned_alloc(64, sizeof(Access *) * MAX_ROW_PER_TXN);
	accesses = (Access **) malloc(sizeof(Access *) * MAX_ROW_PER_TXN);
	for (int i = 0; i < MAX_ROW_PER_TXN; i++)
		accesses[i] = NULL;
	num_accesses_alloc = 0;
#if CC_ALG == TICTOC || CC_ALG == SILO
	_pre_abort = (g_params["pre_abort"] == "true");
	if (g_params["validation_lock"] == "no-wait")
		_validation_no_wait = true;
	else if (g_params["validation_lock"] == "waiting")
		_validation_no_wait = false;
	else 
		assert(false);
#endif
#if CC_ALG == TICTOC
	_max_wts = 0;
	_write_copy_ptr = (g_params["write_copy_form"] == "ptr");
	_atomic_timestamp = (g_params["atomic_timestamp"] == "true");
#elif CC_ALG == SILO
	_cur_tid = 0;
#endif

	_log_entry = new char [g_max_log_entry_size_enc]; //g_max_log_entry_size
	_log_entry_size = 0;

}

void txn_man::set_txn_id(txnid_t txn_id) {
	this->txn_id = txn_id;
}

txnid_t txn_man::get_txn_id() {
	return this->txn_id;
}

workload * txn_man::get_wl() {
	return h_wl;
}

uint64_t txn_man::get_thd_id() {
	return h_thd->get_thd_id();
}

void txn_man::set_ts(ts_t timestamp) {
	this->timestamp = timestamp;
}

ts_t txn_man::get_ts() {
	return this->timestamp;
}

#include <algorithm>

void txn_man::cleanup(RC rc) {
#if CC_ALG == HEKATON
	row_cnt = 0;
	wr_cnt = 0;
	insert_cnt = 0;
	return;
#endif
#if PROFILING
    uint64_t starttime = get_enc_time();
#endif
    PAGE_ENC ** pages = new PAGE_ENC* [row_cnt];
    for (int i = 0; i < row_cnt; i ++) {
        Access *access = accesses[i];
        pages[i] = (PAGE_ENC*) access->orig_row->from_page;
    }
    std::sort(pages, pages+row_cnt);    // cache friendly.

#if VERI_TYPE == MERKLE_TREE
    for (int i = 0; i < row_cnt; i++) {
        Access *access = accesses[i];
//        if (access->type == WR)
//            pages[i]->from->merkle_update(pages[i]);
        pages[i]->from->release_up_cache(pages[i]);
    }
//    for (int i = 0; i < row_cnt; i ++) {
//        pages[i]->from->release_root_latch(get_thd_id());
//    }
#else
    for (int i = 0; i < row_cnt; i++) {
//        if (i == 0 || pages[i] != pages[i-1])
            pages[i]->from->release_up_cache(pages[i]);
    }
#endif

#if USE_LOG == 1
	int size = 0, buf_size = 0;
	// LogRecord** logs = log_generate.createRecords(this);
	// log_generate.createRecords(this);
	// log_generate.create_log_entry(this);
	create_log_entry();
	size = wr_cnt + 1;
	buf_size = size * sizeof(LogRecord);
	// char* buf = log_generate.log_to_buf(logs, size, &buf_size);
	// std::string send_log((char*)log_buf, buf_size);
	// send_logs(send_log, buf_size);
	std::string send_log((char*) _log_entry, _log_entry_size);
	send_logs(send_log, _log_entry_size, get_thd_id());
	// for (int i = 0; i < size; i++) {
	// 	free(logs[i]);
	// }
	// free(logs);
	// free(buf);
#endif
	for (int rid = row_cnt - 1; rid >= 0; rid --) {
		row_t * orig_r = accesses[rid]->orig_row;
		access_t type = accesses[rid]->type;
		if (type == WR && rc == Abort)
			type = XP;

#if (CC_ALG == NO_WAIT || CC_ALG == DL_DETECT) && ISOLATION_LEVEL == REPEATABLE_READ
		if (type == RD) {
			accesses[rid]->data = NULL;
			continue;
		}
#endif

		if (ROLL_BACK && type == XP &&
					(CC_ALG == DL_DETECT || 
					CC_ALG == NO_WAIT || 
					CC_ALG == WAIT_DIE)) 
		{
			orig_r->return_row(type, this, accesses[rid]->orig_data);
		} else {
			orig_r->return_row(type, this, accesses[rid]->data);
		}
#if CC_ALG != TICTOC && CC_ALG != SILO
		accesses[rid]->data = NULL;
#endif
	}

	if (rc == Abort) {
		for (UInt32 i = 0; i < insert_cnt; i ++) {
			row_t * row = insert_rows[i];
			assert(g_part_alloc_enc == false);
#if CC_ALG != HSTORE && CC_ALG != OCC
			free(row->manager);
#endif
			row->free_row();
			free(row);
		}
	}
// batch update hash
	row_cnt = 0;
	wr_cnt = 0;
	insert_cnt = 0;
#if CC_ALG == DL_DETECT
	dl_detector.clear_dep(get_txn_id());
#endif
#if PROFILING
	uint64_t timespan = get_enc_time() - starttime;
    INC_STATS_ENC(get_thd_id(), time_cache,  timespan)
#endif
}

row_t * txn_man::get_row(row_t * row, access_t type) {
	if (CC_ALG == HSTORE)
		return row;
#if PROFILING
	uint64_t starttime = get_enc_time();
#endif
	RC rc = RCOK;
//    assert(accesses[row_cnt] == NULL || accesses[row_cnt]->orig_data != NULL);
	if (accesses[row_cnt] == NULL) {
		// Access * access = (Access *) aligned_alloc(64, sizeof(Access));
		Access * access = (Access *) malloc(sizeof(Access));
		accesses[row_cnt] = access;
#if (CC_ALG == SILO || CC_ALG == TICTOC)
		// access->data = (row_t *) aligned_alloc(64, sizeof(row_t));
		access->data = (row_t *) malloc(sizeof(row_t));
		access->data->init(MAX_TUPLE_SIZE);
		// access->orig_data = (row_t *) aligned_alloc(64, sizeof(row_t));
		access->orig_data = (row_t *) malloc(sizeof(row_t));
		access->orig_data->init(MAX_TUPLE_SIZE);
#elif (CC_ALG == DL_DETECT || CC_ALG == NO_WAIT || CC_ALG == WAIT_DIE)
		// access->orig_data = (row_t *) aligned_alloc(64, sizeof(row_t));
		access->orig_data = (row_t *) malloc(sizeof(row_t));
		access->orig_data->init(MAX_TUPLE_SIZE);
#endif
		num_accesses_alloc ++;
	}
	
	rc = row->get_row(type, this, accesses[ row_cnt ]->data);


	if (rc == Abort) {
		return NULL;
	}
	accesses[row_cnt]->type = type;
	accesses[row_cnt]->orig_row = row;
    accesses[row_cnt]->orig_pg_version = ((PAGE_ENC*) row->from_page)->version;
#if CC_ALG == TICTOC
	accesses[row_cnt]->wts = last_wts;
	accesses[row_cnt]->rts = last_rts;
#elif CC_ALG == SILO
	accesses[row_cnt]->tid = last_tid;
#elif CC_ALG == HEKATON
	accesses[row_cnt]->history_entry = history_entry;
#endif

#if ROLL_BACK && (CC_ALG == DL_DETECT || CC_ALG == NO_WAIT || CC_ALG == WAIT_DIE)
	if (type == WR) {
        // No orig_data here??? why?
		accesses[row_cnt]->orig_data->table = row->get_table();
		accesses[row_cnt]->orig_data->copy(row);
	}
#endif

#if (CC_ALG == NO_WAIT || CC_ALG == DL_DETECT) && ISOLATION_LEVEL == REPEATABLE_READ
	if (type == RD)
		row->return_row(type, this, accesses[ row_cnt ]->data);
#endif
	
	row_cnt ++;
	if (type == WR)
		wr_cnt ++;

#if PROFILING
	uint64_t timespan = get_enc_time() - starttime;
	INC_TMP_STATS_ENC(get_thd_id(), time_man, timespan);
#endif
	return accesses[row_cnt - 1]->data;
}

RC txn_man::insert_row(row_t * row, std::string iname) {
	if (CC_ALG == HSTORE)
		return RCOK;
#if INDEX_STRUCT != IDX_BTREE or !FULL_TPCC
    assert(insert_cnt < MAX_ROW_PER_TXN);
	insert_rows[insert_cnt ++] = row;
	return RCOK;
    assert(false);
#endif
//    assert(row);
    auto * item = new itemid_t(DT_row, row);
    // index --> en_index;
//    auto idx = (IndexHash *) inner_index_map->_indexes[iname];
//    assert(idx);
//    idx->index_insert(row->get_primary_key(), item, row->get_part_id());
    INDEX_ENC * index_enc = (INDEX_ENC *) tab_map->_indexes[iname];
    index_enc->index_insert(row->get_primary_key(), item, row->get_part_id());
//    auto new_row = (row_t*)item->location;
//    index_enc->release_up_cache((PAGE_ENC*)new_row->from_page);
//	assert(insert_cnt < MAX_ROW_PER_TXN);
//	insert_rows[insert_cnt ++] = row;
    return Abort;
}

itemid_t *
txn_man::index_read(std::string iname, idx_key_t key, int part_id) {
	uint64_t starttime = get_enc_time();
	itemid_t * item = nullptr;
	// index --> en_index;
    INDEX_ENC * index_enc = (INDEX_ENC *) tab_map->_indexes[iname];
	if (index_enc == nullptr) {
        assert(false);
//        index_enc = new IndexEnc;
//        index_enc->init(index->_bucket_cnt, int(index->_part_cnt));
//        tab_map->_indexes[index->index_name] = index_enc;
	}
#if INDEX_STRUCT == IDX_HASH
	RC rc = index_enc->index_read(iname, key, item, part_id, get_thd_id());
#else
    RC rc = index_enc->index_read(key, item, part_id, get_thd_id());
#endif
    INC_TMP_STATS_ENC(get_thd_id(), time_index, get_enc_time() - starttime);
    if (rc == ERROR) {
        return (itemid_t*)(0x1);
    }
    if (rc != RCOK) {
        return nullptr;
    }
    assert(((row_t*)item->location)->get_table());

	return item;
}

itemid_t *
txn_man::index_next(std::string iname, itemid_t* last) {
#if PROFILING
	uint64_t starttime = get_enc_time();
#endif
//    assert(false);
    itemid_t * item;
    assert(last != nullptr);
    // index --> en_index;
    INDEX_ENC * index_enc = (INDEX_ENC *) tab_map->_indexes[iname];
    if (index_enc == nullptr) {
        assert(false);
    }
#if INDEX_STRUCT == IDX_HASH
    assert(false);
#else
    index_enc->index_next(item, last);
#endif
    assert(((row_t*)item->location)->get_table());
    // index->index_read(key, item, part_id, get_thd_id());
#if PROFILING
	INC_TMP_STATS_ENC(get_thd_id(), time_index, get_enc_time() - starttime);
#endif
    return item;
}

void 
txn_man::index_read(std::string iname, idx_key_t key, int part_id, itemid_t *& item) {
#if PROFILING
	uint64_t starttime = get_enc_time();
#endif
	INDEX_ENC * index_enc = (INDEX_ENC *) tab_map->_indexes[iname];
    if (index_enc == nullptr) {
        assert(false);
//        index_enc = new IndexEnc;
//        index_enc->init(index->_bucket_cnt, int(index->_part_cnt));
//        tab_map->_indexes[index->index_name] = index_enc;
    }
#if INDEX_STRUCT == IDX_HASH
    RC rc = index_enc->index_read(iname, key, item, part_id, get_thd_id());
    if (rc == ERROR) {
        assert(false);
    }
#else
    index_enc->index_read(key, item, part_id, get_thd_id());
#endif
	// index->index_read(key, item, part_id, get_thd_id());
#if PROFILING
	INC_TMP_STATS_ENC(get_thd_id(), time_index, get_enc_time() - starttime);
#endif
}

RC txn_man::finish(RC rc) {
#if PROFILING
	uint64_t t1 = get_enc_time();

	// for (int i = 0; i < 10; i ++) {
	// 	// char* buf = (char*)malloc(1000);
	// 	char* buf = new char[1000];
	// 	// free(buf);
	// 	delete buf;
	// }
	 uint64_t t2 = get_enc_time();
	uint64_t t3 = get_enc_time();
	INC_TMP_STATS_ENC(0, time_index, t2-t1);
	INC_TMP_STATS_ENC(0, time_man, t3-t2);
#endif
#if FULL_TPCC
    PAGE_ENC ** pages = new PAGE_ENC* [row_cnt];
    for (int i = 0; i < row_cnt; i ++) {
        Access *access = accesses[i];
        pages[i] = (PAGE_ENC*) access->orig_row->from_page;
        if (access->orig_pg_version != pages[i]->version) {
            return finish(Abort);
        }
    }
#endif
#if CC_ALG == HSTORE
	return RCOK;
#endif
#if PROFILING
	uint64_t starttime = get_enc_time();
#endif
#if CC_ALG == OCC
	if (rc == RCOK)
		rc = occ_man.validate(this);
	else 
		cleanup(rc);
#elif CC_ALG == TICTOC
	if (rc == RCOK)
		rc = validate_tictoc();
	else 
		cleanup(rc);
#elif CC_ALG == SILO
	if (rc == RCOK)
		rc = validate_silo();
	else 
		cleanup(rc);
#elif CC_ALG == HEKATON
	rc = validate_hekaton(rc);
	cleanup(rc);
#else
	if (rc == RCOK) {
        PAGE_ENC ** pages = new PAGE_ENC* [row_cnt];
        for (int i = 0; i < row_cnt; i ++) {
            Access *access = accesses[i];
            pages[i] = (PAGE_ENC*) access->orig_row->from_page;
        }
        std::sort(pages, pages+row_cnt);    // cache friendly.
#if INDEX_STRUCT == IDX_HASH
        commit_t = get_enc_time();
        bool synced = false;
        for (int i = 0; i < row_cnt; i++) {
            if (i == 0 || pages[i] != pages[i-1]) synced = false;
            if (!synced) {
                pages[i]->from->sync_version(pages[i], commit_t, begin_t, accesses[i]->type == WR);
                    // propagate the page version.
                synced = true;
            }
        }
//	delete pages;
#endif
    }
    cleanup(rc);
#endif
#if PROFILING
	uint64_t timespan = get_enc_time() - starttime;
	INC_TMP_STATS_ENC(get_thd_id(), time_man,  timespan);
	INC_STATS_ENC(get_thd_id(), time_cleanup,  timespan);
#endif
	return rc;
}

void
txn_man::release() {
	for (int i = 0; i < num_accesses_alloc; i++)
		free(accesses[i]);
	free(accesses);
}


itemid_t* txn_man::index_read(INDEX* index, idx_key_t key, int part_id) {
    return index_read(std::string(index->index_name), key, part_id);
}

void txn_man::index_read(INDEX* index, idx_key_t key, int part_id, itemid_t *& item) {
    index_read(std::string(index->index_name), key, part_id, item);
}


void txn_man::create_log_entry() {

#if LOG_TYPE == LOG_DATA

	// Format for serial logging
	// | checksum:4 | size:4 | N:4 | (part_id:8 | pgid:8 | offset:8 | primary_key:8 | data_length:4 | data:?) * N

	uint32_t offset = 0;
	uint32_t checksum = 0xbeef;  // we also use this to distinguish PSN items and log items
	uint32_t num_keys = 0;
	PACK(_log_entry, checksum, offset);
    offset += sizeof(uint32_t); // make space for size;
    offset += sizeof(uint32_t); // make space for N;
	// std::cout << "num_keys:" << row_cnt << std::endl;

  	for (uint32_t i = 0; i < row_cnt; i ++) {
    	Access * access = accesses[i];
    	if (access->type != WR) continue;
    	num_keys ++;
		// row_t * orig_row = accesses[write_set[i]]->orig_row; 
		// uint32_t table_id = access->orig_row->get_table()->get_table_id();
    	// uint32_t table_id = 0;
		uint64_t key = access->orig_row->get_primary_key();
		uint32_t tuple_size = access->orig_row->get_tuple_size();
		char * tuple_data = access->orig_row->data;
		//assert(tuple_size!=0);

		uint64_t part_id = (uint64_t) (((PAGE_ENC*)access->orig_row->from_page)->part);
#if INDEX_STRUCT == IDX_BTREE
        uint64_t bkt_id = ((PAGE_ENC*)access->orig_row->from_page)->node_id;
#else
        uint64_t bkt_id = ((PAGE_ENC*)access->orig_row->from_page)->bkt;
		uint64_t commit_ts = ((PAGE_ENC*)access->orig_row->from_page)->from->_verify_hash[part_id][bkt_id]->get_max_ts();
#endif

#if INDEX_STRUCT == IDX_HASH
        PACK(_log_entry, part_id, offset);        // part.
        PACK(_log_entry, bkt_id, offset);        // page id.
        PACK(_log_entry, commit_ts, offset);
        PACK(_log_entry, access->orig_row->offset, offset); // invalid data.
        assert(access->orig_row->offset == 0);
#else
        PACK(_log_entry, part_id, offset);        // part.
        PACK(_log_entry, ((PAGE_ENC*)access->orig_row->from_page)->node_id, offset);        // page id.
        PACK(_log_entry, access->orig_row->offset, offset); // the offset of the item_t in data.
#endif
		// PACK(_log_entry, table_id, offset);
		PACK(_log_entry, key, offset);
		PACK(_log_entry, tuple_size, offset);
		PACK_SIZE(_log_entry, tuple_data, tuple_size, offset);
	}

  	_log_entry_size = offset;
	assert(_log_entry_size < g_max_log_entry_size_enc); // g_max_log_entry_size
  // update size. 
    memcpy(_log_entry + sizeof(uint32_t), &_log_entry_size, sizeof(uint32_t));
    memcpy(_log_entry + 2*sizeof(uint32_t), &num_keys, sizeof(uint32_t));
  //cout << _log_entry_size << endl;

#elif LOG_TYPE == LOG_COMMAND

	// Format for serial logging
	// 	| checksum | size | benchmark_specific_command | 

	uint32_t offset = 0;
	uint32_t checksum = 0xbeef;
	uint32_t size = 0;
	PACK(_log_entry, checksum, offset);
	PACK(_log_entry, size, offset);

  	_log_entry_size = offset;
	// internally, the following function will update _log_entry_size and _log_entry
	get_cmd_log_entry();
	
	assert(_log_entry_size < g_max_log_entry_size_enc); // g_max_log_entry_size
	assert(_log_entry_size > sizeof(uint32_t) * 2);
	memcpy(_log_entry + sizeof(uint32_t), &_log_entry_size, sizeof(uint32_t));
  // INC_FLOAT_STATS(log_total_size, _log_entry_size);
	// INC_INT_STATS_V0(num_log_entries, 1);

#else
	assert(false);
#endif

}