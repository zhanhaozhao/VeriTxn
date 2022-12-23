#include "global.h"
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
#include "manager_enc.h"
// #include "common/mem_alloc.h"
#include "common/base_query.h"
#include "re_ycsb_txn.h"

void re_ycsb_txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
	re_txn_man::init(h_thd, h_wl, thd_id);
	_wl = (ycsb_wl *) h_wl;
}



void 
re_ycsb_txn_man::recover_txn(char * log_entry, uint64_t tid)
{
	uint64_t tt = get_sys_clock();
#if LOG_TYPE == LOG_DATA
	// Format 
	// | N | (table_id | primary_key | data_length | data) * N
	// predecessor_info has the following format
	//   | num_raw_preds | raw_preds | num_waw_preds | waw_preds
	uint32_t offset = 0;
	uint32_t num_keys; 
	UNPACK(log_entry, num_keys, offset);
	for (uint32_t i = 0; i < num_keys; i ++) {
		uint32_t table_id;
		uint64_t key;
		uint32_t data_length;
		char * data;

		UNPACK(log_entry, table_id, offset);
		UNPACK(log_entry, key, offset);
		UNPACK(log_entry, data_length, offset);
		data = log_entry + offset;
		offset += data_length;
		
		// Serial has log streams corresponding to the dependency order
		// itemid_t * m_item = index_read(_wl->the_index, key, 0);
		// base_row_t * row = ((base_row_t *)m_item->location);
		// row->set_data(data, data_length);
	}
#elif LOG_TYPE == LOG_COMMAND
	// Format
	//  | stored_procedure_id | num_keys | (key, type) * num_keys
	if (!_query) {
		// these are only executed once. 
		_query = new ycsb_query;
		_query->request_cnt = 0;
		_query->requests = new ycsb_request [g_req_per_query];
	}
	uint32_t offset = sizeof(uint32_t);
	UNPACK(log_entry, _query->request_cnt, offset);
	for (uint32_t i = 0; i < _query->request_cnt; i ++) {
		UNPACK(log_entry, _query->requests[i].key, offset);
		UNPACK(log_entry, _query->requests[i].rtype, offset);
	}
//	uint64_t tt = get_sys_clock();
    uint64_t ttrt = get_sys_clock();
	// run_txn(_query, true); // TODO!!!zhanhao
	INC_INT_STATS(time_debug8, get_sys_clock() - ttrt);
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
