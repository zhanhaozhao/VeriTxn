
#ifndef LOGGER_H
#define LOGGER_H

#include "global.h"
#include "helper.h"
#include "log.h"
// #include "concurrentqueue.h"
#include <set>
#include <queue>
#include <fstream>


class Logger {
public:
  void init();
  void release();

  void enqueueRecord(LogRecord* record);
  void processRecord(uint64_t thd_id);
private:
  pthread_mutex_t mtx;
  uint64_t lsn;

  std::queue<LogRecord*> log_queue;
  const char * log_file_name;
  std::ofstream log_file;
  uint64_t aries_write_offset;
  std::set<uint64_t> txns_to_notify;
  uint64_t last_flush;
  uint64_t log_buf_cnt;
};


#endif
