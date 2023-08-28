#include "global.h"
#include "global_struct.h"
#include "common/helper.h"
#include "ycsb.h"
#include "ycsb_query.h"
#include "wl.h"
#include "db_thread.h"
#include "table.h"
#include "base_row.h"
#include "index_hash.h"
#include "index_btree.h"
#include "catalog.h"
// #include "common/mem_alloc.h"
#include "common/base_query.h"
#include "re_ycsb_txn.h"
#include "kvengine.h"

void re_ycsb_txn_man::init(RPCThread * h_thd, workload * h_wl, uint64_t thd_id) {
	re_txn_man::init(h_thd, h_wl, thd_id);
	_wl = (ycsb_wl *) h_wl;
}


RC re_ycsb_txn_man::run_txn(base_query * query) {
	RC rc;
	ycsb_query * m_query = (ycsb_query *) query;
	_query = m_query;
	ycsb_wl * wl = (ycsb_wl *) h_wl;
	itemid_t * m_item = NULL;
  	row_cnt = 0;

#if USE_SGX != 1
	rocksdb::WriteBatch batch;

	for (uint32_t rid = 0; rid < m_query->request_cnt; rid ++) {
		ycsb_request * req = &m_query->requests[rid];
		int part_id = wl->key_to_part( req->key );
		bool finish_req = false;
		UInt32 iteration = 0;
		while ( !finish_req ) {
			if (iteration == 0) {
				// m_item = index_read("MAIN_INDEX", req->key, part_id);
			} 
// #if INDEX_STRUCT == IDX_BTREE
// 			else {
// 				_wl->the_index->index_next(get_thd_id(), m_item);
// 				if (m_item == NULL)
// 					break;
// 			}
// #endif
			// row_t * row = ((row_t *)m_item->location);
			// row_t * row_local; 
			// access_t type = req->rtype;

			std::string data;
			char * rockskey = new char [50];
        	sprintf(rockskey, "%lu_%lu",  req->key, part_id);
			const auto key_slice = rocksdb::Slice(rockskey);
			eng->DBGet(key_slice, &data);

            // assert(row->get_table());
			// row_local = get_row(row, type);
			// if (row_local == NULL) {
				// rc = Abort;
				// goto final;
			// }

			// Computation //
			// Only do computation when there are more than 1 requests.
            if (m_query->request_cnt > 1) {
                if (req->rtype == RD || req->rtype == SCAN) {
//                  for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						// char * data = row_local->get_data(); //readfrom rocksdb
						// __attribute__((unused)) uint64_t fval = *(uint64_t *)(&data[fid * 10]);
//                  }
                } else {
					// std::cout << req->rtype << std::endl;
                    // assert(req->rtype == WR || req->rtype == XP);
//					for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						// char * data = malloc (10); // readfrom rocksdb
						// char * data = row->get_data();
						// *(uint64_t *)(&data[fid * 10]) = 0;
						batch.Put(rockskey, data);
						// eng->DBPut(rockskey, data);

//					}
                } 
            }
			iteration ++;
			if (req->rtype == RD || req->rtype == WR || iteration == req->scan_len)
				finish_req = true;
		}
	}
	rc = RCOK;

#endif

final:
	assert (g_log_recover);
	// construct writebatch and write to rocksdb
#if USE_SGX != 1
	eng->DBWrite(&batch);
	// std::cout << "num_keys:" << num_keys << std::endl;
#endif
	return RCOK;
}

void 
re_ycsb_txn_man::recover_txn(char * log_entry, uint64_t tid)
{
	uint64_t tt = get_sys_clock();
#if LOG_TYPE == LOG_DATA
	// Format
    // | checksum:4 | size:4 | N:4 | (part_id:8 | pgid:8 | offset:8 | primary_key:8 | data_length:4 | data:?) * N
	// predecessor_info has the following format
	//   | num_raw_preds | raw_preds | num_waw_preds | waw_preds
	uint32_t offset = 0;

	uint32_t checksum;
	UNPACK(log_entry, checksum, offset);
    assert(checksum == 0xbeef);
	uint32_t entrysize;
	UNPACK(log_entry, entrysize, offset);
	uint32_t num_keys; 
	UNPACK(log_entry, num_keys, offset);

#if USE_SGX != 1
	rocksdb::WriteBatch batch;
#endif

	for (uint32_t i = 0; i < num_keys; i ++) {
		uint64_t part_id, node_id, page_offset, ts;
		uint64_t key;
		uint32_t data_length;
		char * rockskey;
		char * data;

		UNPACK(log_entry, part_id, offset);
        UNPACK(log_entry, node_id, offset);
        UNPACK(log_entry, ts, offset);
		UNPACK(log_entry, page_offset, offset);
#if INDEX_STRUCT == IDX_HASH and WORKLOAD == YCSB
        assert(part_id == 0 && page_offset == 0);
#endif

        rockskey = new char [50];
        // multiple version.
//        sprintf(rockskey, "%lu_%lu_%lu_%lu", part_id, node_id, page_offset, ts);
        sprintf(rockskey, "%lu_%lu_%lu", part_id, node_id, page_offset);
		UNPACK(log_entry, key, offset);
		
        UNPACK(log_entry, data_length, offset);
		data = log_entry + offset;
		offset += data_length;
		if (offset > entrysize) return;
        // assert(offset <= entrysize);
		
		// Serial has log streams corresponding to the dependency order
		// itemid_t * m_item = index_read(_wl->the_index, key, 0);
		// base_row_t * row = ((base_row_t *)m_item->location);
		// row->set_data(data, data_length);
		
#if USE_SGX != 1
		batch.Put(rockskey, data);
		// eng->DBPut(rockskey, data);
#endif
	}

#if USE_SGX != 1
	eng->DBWrite(&batch);
	// std::cout << "num_keys:" << num_keys << std::endl;
#endif
	

#elif LOG_TYPE == LOG_COMMAND
	// Format
	// | checksum:4 | size:4 | stored_procedure_id:4 | num_keys:4 | (key, type) * num_keys
	if (!_query) {
		// these are only executed once. 
		_query = new ycsb_query;
		_query->request_cnt = 0;
		_query->requests = new ycsb_request [g_req_per_query];
	}
	uint32_t offset = sizeof(uint32_t) * 3;
	UNPACK(log_entry, _query->request_cnt, offset);
	for (uint32_t i = 0; i < _query->request_cnt; i ++) {
		UNPACK(log_entry, _query->requests[i].key, offset);
		UNPACK(log_entry, _query->requests[i].rtype, offset);
	}
//	uint64_t tt = get_sys_clock();
    uint64_t ttrt = get_sys_clock();
	run_txn(_query);

	// INC_INT_STATS(time_debug8, get_sys_clock() - ttrt);
//	INC_STATS(GET_THD_ID, debug8, get_sys_clock() - tt);

/*	#if LOG_ALGORITHM == LOG_PARALLEL
		this->_recover_state = recover_state;
	#endif
	for (uint32_t i = 0; i < num_keys; i ++) {

		itemid_t * m_item = index_read(_wl->the_index, key, 0);
		row_t * row = ((row_t *)m_item->location);
			
		assert(row);
		char * data = row->get_data(this, rtype);
		assert(data);
		// Computation //
		if (rtype == RD || rtype == SCAN) {
			for (uint32_t i = 0; i < _wl->the_table->get_schema()->get_field_cnt(); i++) { 
				__attribute__((unused)) char * value = row_t::get_value(
					_wl->the_table->get_schema(), i, data);
			}
		} else {
			//char value[100] = "value\n";
			assert(rtype == WR);
			for (uint32_t i = 0; i < _wl->the_table->get_schema()->get_field_cnt(); i++) { 
				char * value = row_t::get_value(_wl->the_table->get_schema(), i, data);
				for (uint32_t j = 0; j < _wl->the_table->get_schema()->get_field_size(i); j ++) 
					value[j] = value[j] + 1;
				row_t::set_value(_wl->the_table->get_schema(), i, data, value);
			}
		} 
	}
		if (rtype == RD || rtype == SCAN) {
			__attribute__((unused)) uint64_t fval = *(uint64_t *)(&data[0]);
		} else {
			uint64_t fval = *(uint64_t *)(&data[0]);
			*(uint64_t *)(&data[0]) = fval + 1;
		} 
	}*/
#else
	assert(false);
#endif
	// INC_INT_STATS(time_recover_txn, get_sys_clock() - tt);
}
