#include "logger.h"
// #include "work_queue.h"
#include "message.h"
// #include "mem_alloc.h"
#include <fstream>


void Logger::init() {
  // this->log_file_name = log_file_name;
  // log_file.open(log_file_name, ios::out | ios::app | ios::binary);
  // assert(log_file.is_open());
  pthread_mutex_init(&mtx,NULL);
}

void Logger::release() { log_file.close(); }

// void LogRecord::copyRecord(LogRecord* record) {
//   rcd.init();
//   rcd.lsn = record->rcd.lsn;
//   rcd.iud = record->rcd.iud;
//   rcd.type = record->rcd.type;
//   rcd.txn_id = record->rcd.txn_id;
//   rcd.table_id = record->rcd.table_id;
//   rcd.key = record->rcd.key;
// }


void Logger::enqueueRecord(LogRecord* record) {
  // DEBUG("Enqueue Log Record %ld\n",record->rcd.txn_id);
  pthread_mutex_lock(&mtx);
  log_queue.push(record);
  pthread_mutex_unlock(&mtx);
}

void Logger::processRecord(uint64_t thd_id) {
  if (log_queue.empty()) return;
  LogRecord * record = NULL;
  pthread_mutex_lock(&mtx);
  if(!log_queue.empty()) {
    record = log_queue.front();
    log_queue.pop();
  }
  pthread_mutex_unlock(&mtx);

  if(record) {
    // TODO: 将日志发给云服务器
    // uint64_t starttime = get_sys_clock();
    // DEBUG("Dequeue Log Record %ld\n",record->rcd.txn_id);
    // if(record->rcd.iud == L_NOTIFY) {
    //   flushBuffer(thd_id);
    //   work_queue.enqueue(thd_id,Message::create_message(record->rcd.txn_id,LOG_FLUSHED),false);

    // }
    // writeToBuffer(thd_id,record);
    // //writeToBuffer((char*)(&record->rcd),sizeof(record->rcd));
    // log_buf_cnt++;
    // mem_allocator.free(record,sizeof(LogRecord));
    // INC_STATS(thd_id,log_process_time,get_sys_clock() - starttime);
  }

}
