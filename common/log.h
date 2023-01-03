
#ifndef LOGGER_TYPE_H
#define LOGGER_TYPE_H

// #include "global_enc.h"
#include "helper.h"


enum LogRecType {
  LRT_INVALID = 0,
  LRT_INSERT,
  LRT_UPDATE,
  LRT_DELETE,
  LRT_TRUNCATE
};
enum LogIUD {
  L_INSERT = 0,
  L_UPDATE,
  L_DELETE,
  L_NOTIFY
};


// ARIES-style log record (physiological logging)
struct AriesLogRecord {
  void init() {
    checksum = 0;
    lsn = UINT64_MAX;
    type = LRT_UPDATE;
    iud = L_UPDATE;
    txn_id = UINT64_MAX;
    table_id = 0;
    key = UINT64_MAX;
  }

  uint32_t checksum;
  uint64_t lsn;
  LogRecType type;
  LogIUD iud;
  uint64_t txn_id; // transaction id
  //uint32_t partid; // partition id
  uint32_t table_id; // table being updated
  uint64_t key; // primary key (determines the partition ID)
};

class LogRecord {
public:
  //LogRecord();
  LogRecType getType() { return rcd.type; }
  // TODO: compute a reasonable checksum
  uint64_t computeChecksum() {
    return (uint64_t)rcd.txn_id;
  };
  AriesLogRecord rcd;
private:
  bool isValid;

};
#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
class Logqueue {
public:
  void init() {
    head = 0;
    tail = 0;
  }
  void enqueue(LogRecord** records, int size);
  LogRecord* dequeue();
private:
  LogRecord _records[MAX_LOG_QUEUE_RECORDS];
  int head;
  int tail;
};
#endif

#endif

