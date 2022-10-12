#include "log.h"
// #include <cassert>
#if LOG_QUEUE_TYPE == LOG_CIRCUL_BUFF
void Logqueue::enqueue(LogRecord** records, int size) {
  for (int i = 0; i < size; i++) {
    // assert(tail >= head);
    while (tail - head >=  MAX_LOG_QUEUE_RECORDS);
    _records[tail].rcd = records[i]->rcd;
    tail++;
  }
}

LogRecord* Logqueue::dequeue() {
  if (head >= tail) return nullptr;
  LogRecord* newrecord = new LogRecord;
  newrecord->rcd = _records[head].rcd;
  head++;
  return newrecord;
}
#endif