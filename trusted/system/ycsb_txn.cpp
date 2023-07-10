#include "global_enc.h"
#include "common/helper.h"
#include "ycsb.h"
#include "ycsb_query.h"
#include "wl.h"
#include "db_thread.h"
#include "table.h"
#include "row_enc.h"
#include "index_hash.h"
#include "index_btree.h"
#include "catalog.h"
#include "manager_enc.h"
#include "row_lock.h"
#include "row_ts.h"
#include "row_mvcc.h"
// #include "common/mem_alloc.h"
#include "common/base_query.h"
#include "ycsb_txn.h"

void ycsb_txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
	txn_man::init(h_thd, h_wl, thd_id);
	_wl = (ycsb_wl *) h_wl;
}

RC ycsb_txn_man::run_txn(base_query * query) {
	RC rc;
	ycsb_query * m_query = (ycsb_query *) query;
	_query = m_query;
	ycsb_wl * wl = (ycsb_wl *) h_wl;
	itemid_t * m_item = NULL;
  	row_cnt = 0;

	for (uint32_t rid = 0; rid < m_query->request_cnt; rid ++) {
		ycsb_request * req = &m_query->requests[rid];
		int part_id = wl->key_to_part( req->key );
		bool finish_req = false;
		UInt32 iteration = 0;
		while ( !finish_req ) {
			if (iteration == 0) {
				m_item = index_read("MAIN_INDEX", req->key, part_id);
                if (m_item == nullptr) {
                    rc = Abort;
                    goto final;
                }
                if (m_item == (itemid_t*)0x1) {
                    rc = finish(Abort);
                    return ERROR;
                }
            }
#if INDEX_STRUCT == IDX_BTREE
			else {
                m_item = index_next("MAIN_INDEX", m_item);
				if (m_item == NULL)
					break;
			}
#endif
			row_t * row = ((row_t *)m_item->location);
			row_t * row_local; 
			access_t type = req->rtype;
            assert(row->get_primary_key() == req->key);
            assert(row->get_table());
			row_local = get_row(row, type);
			if (row_local == NULL) {
				rc = Abort;
				goto final;
			}

			// Computation //
			// Only do computation when there are more than 1 requests.
            if (m_query->request_cnt > 1) {
                if (req->rtype == RD || req->rtype == SCAN) {
//                  for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						char * data = row_local->get_data();
						__attribute__((unused)) uint64_t fval = *(uint64_t *)(&data[fid * 10]);
//                  }
                } else {
                    assert(req->rtype == WR);
//					for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						char * data = row->get_data();
						*(uint64_t *)(&data[fid * 10]) = 0;
//					}
                } 
            }


			iteration ++;
			if (req->rtype == RD || req->rtype == WR || iteration == req->scan_len)
				finish_req = true;
		}
	}
	rc = RCOK;
final:
	rc = finish(rc);
	return rc;
}

void 
ycsb_txn_man::get_cmd_log_entry()
{
	// Format
	//  | stored_procedure_id | num_keys | (key, type) * numk_eys
	uint32_t sp_id = 0;
	uint32_t num_keys = _query->request_cnt;

	PACK(_log_entry, sp_id, _log_entry_size);
	PACK(_log_entry, num_keys, _log_entry_size);
	for (uint32_t i = 0; i < num_keys; i ++) {
		PACK(_log_entry, _query->requests[i].key, _log_entry_size);
		PACK(_log_entry, _query->requests[i].rtype, _log_entry_size);
	}
}
