#include "logger_enc.h"
#include "row_enc.h"
#include <fstream>


LogRecord** Logger_generate::createRecords(txn_man* txn) {
  LogRecord ** records = (LogRecord**)malloc(sizeof(LogRecord*) * (txn->wr_cnt + 1));
  
  int id = 0;
  for (int i = 0; i < txn->row_cnt; i++) {
    Access * access = txn->accesses[i];
    if (access->type != WR) continue;

    records[id] = (LogRecord*)malloc(sizeof(LogRecord));
    records[id]->rcd.init();
    records[id]->rcd.lsn = ATOM_FETCH_ADD(lsn,1);
    
    records[id]->rcd.iud = L_UPDATE;
    records[id]->rcd.txn_id = txn->get_txn_id();
    records[id]->rcd.table_id = 0;
    records[id]->rcd.key = access->orig_row->get_primary_key();
    id++;
  }

  records[id] = (LogRecord*)malloc(sizeof(LogRecord));
  records[id]->rcd.init();
  records[id]->rcd.lsn = ATOM_FETCH_ADD(lsn,1);
  
  records[id]->rcd.iud = L_NOTIFY;
  records[id]->rcd.txn_id = txn->get_txn_id();
  records[id]->rcd.table_id = 0;
  records[id]->rcd.key = 0;

  return records;
}
