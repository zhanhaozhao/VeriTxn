
#ifndef LOGGER_ENC_H
#define LOGGER_ENC_H

#include "global_enc.h"
#include "helper.h"
#include "txn.h"
#include "log.h"
// #include "concurrentqueue.h"
#include <set>
#include <queue>
#include <fstream>

class Logger_generate {
public:
  void init() {
    pthread_mutex_init(&mtx,NULL);
    lsn = 0;
  }
  LogRecord ** createRecords(txn_man* txn_man);
  // LogRecord ** create_log_entry(txn_man* txn_man);
  char* log_to_buf(LogRecord** logs, int size, int *buf_size);
private:
  pthread_mutex_t mtx;
  uint64_t lsn;
};


#endif
