
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
private:
  pthread_mutex_t mtx;
  uint64_t lsn;
};


#endif
