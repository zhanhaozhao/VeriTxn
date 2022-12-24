#include "logger_enc.h"
#include "row_enc.h"
#include <fstream>
#include "helper.h"

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