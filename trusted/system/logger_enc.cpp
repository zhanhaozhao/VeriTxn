#include "logger_enc.h"
#include "row_enc.h"
#include <fstream>
#include "helper.h"


LogRecord** Logger_generate::create_log_entry(txn_man* txn) {

#if LOG_TYPE == LOG_DATA

	// Format for serial logging
	// | checksum:4 | size:4 | N:4 | (table_id:4 | primary_key:8 | data_length:4 | data:?) * N

	uint32_t offset = 0;
	uint32_t checksum = 0xbeef;  // we also use this to distinguish PSN items and log items

	PACK(txn->_log_entry, checksum, offset);

	offset += sizeof(uint32_t); // make space for size;

  PACK(txn->_log_entry, txn->row_cnt, offset);

  for (uint32_t i = 0; i < txn->row_cnt; i ++) {
    Access * access = txn->accesses[i];
    if (access->type != WR) continue;
		// row_t * orig_row = accesses[write_set[i]]->orig_row; 
		// uint32_t table_id = access->orig_row->get_table()->get_table_id();
    uint32_t table_id = 0;
		uint64_t key = access->orig_row->get_primary_key();
		uint32_t tuple_size = access->orig_row->get_tuple_size();
		char * tuple_data = access->orig_row->data;
		//assert(tuple_size!=0);

		PACK(txn->_log_entry, table_id, offset);
		PACK(txn->_log_entry, key, offset);
		PACK(txn->_log_entry, tuple_size, offset);
		PACK_SIZE(txn->_log_entry, tuple_data, tuple_size, offset);
	}

  txn->_log_entry_size = offset;
	assert(txn->_log_entry_size < 16384); // g_max_log_entry_size
  // update size. 
	memcpy(txn->_log_entry + sizeof(uint32_t), &txn->_log_entry_size, sizeof(uint32_t));
  //cout << _log_entry_size << endl;

#else 
  // for command log

	uint32_t offset = 0;
	uint32_t checksum = 0xbeef;
	uint32_t size = 0;
	PACK(txn->_log_entry, checksum, offset);
	PACK(txn->_log_entry, size, offset);

  txn->_log_entry_size = offset;
	// internally, the following function will update _log_entry_size and _log_entry
	get_cmd_log_entry();
	
	assert(txn->_log_entry_size < 16384); // g_max_log_entry_size
	assert(txn->_log_entry_size > sizeof(uint32_t) * 2);
	memcpy(txn->_log_entry + sizeof(uint32_t), &txn->_log_entry_size, sizeof(uint32_t));

#endif

}


LogRecord** Logger_generate::createRecords(txn_man* txn) {
  // LogRecord ** records = (LogRecord**)malloc(sizeof(LogRecord*) * (txn->wr_cnt + 1));
  
  int id = 0;
  for (int i = 0; i < txn->row_cnt; i++) {
    Access * access = txn->accesses[i];
    if (access->type != WR) continue;
    txn->log_buf[id].rcd.init();
    txn->log_buf[id].rcd.lsn = ATOM_FETCH_ADD(lsn,1);
    txn->log_buf[id].rcd.iud = L_UPDATE;
    txn->log_buf[id].rcd.txn_id = txn->get_txn_id();
    txn->log_buf[id].rcd.table_id = 0;
    txn->log_buf[id].rcd.key = access->orig_row->get_primary_key();
    // records[id] = (LogRecord*)malloc(sizeof(LogRecord));
    // records[id]->rcd.init();
    // records[id]->rcd.lsn = ATOM_FETCH_ADD(lsn,1);
    
    // records[id]->rcd.iud = L_UPDATE;
    // records[id]->rcd.txn_id = txn->get_txn_id();
    // records[id]->rcd.table_id = 0;
    // records[id]->rcd.key = access->orig_row->get_primary_key();
    id++;
  }

  // records[id] = (LogRecord*)malloc(sizeof(LogRecord));
  txn->log_buf[id].rcd.init();
  txn->log_buf[id].rcd.lsn = ATOM_FETCH_ADD(lsn,1);
  txn->log_buf[id].rcd.iud = L_NOTIFY;
  txn->log_buf[id].rcd.txn_id = txn->get_txn_id();
  txn->log_buf[id].rcd.table_id = 0;
  txn->log_buf[id].rcd.key = 0;
  // records[id]->rcd.init();
  // records[id]->rcd.lsn = ATOM_FETCH_ADD(lsn,1);
  
  // records[id]->rcd.iud = L_NOTIFY;
  // records[id]->rcd.txn_id = txn->get_txn_id();
  // records[id]->rcd.table_id = 0;
  // records[id]->rcd.key = 0;
  return nullptr;
}

char* Logger_generate::log_to_buf(LogRecord** logs, int size, int *buf_size) {
  #if WORKLOAD == YCSB
    char* buf = new char[sizeof(LogRecord) * (MAX_ROW_PER_TXN + 2)];
  #elif WORKLOAD == TPCC 
    char* buf = new char[sizeof(LogRecord) * (20)];
  #endif
    int buf_p=0;
    for (int i = 0; i < size; i++){
        LogRecord * record = logs[i];
        COPY_BUF(buf,record->rcd.checksum,buf_p);
        COPY_BUF(buf,record->rcd.lsn,buf_p);
        COPY_BUF(buf,record->rcd.type,buf_p);
        COPY_BUF(buf,record->rcd.iud,buf_p);
        COPY_BUF(buf,record->rcd.txn_id,buf_p);
        COPY_BUF(buf,record->rcd.table_id,buf_p);
        COPY_BUF(buf,record->rcd.key,buf_p);
    }
    *buf_size = buf_p;
    return buf;
}