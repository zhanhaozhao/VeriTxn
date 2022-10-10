
#ifndef LOGGER_H
#define LOGGER_H

#include "global.h"
#include "helper.h"
#include "log.h"
// #include "concurrentqueue.h"
#include <set>
#include <queue>
#include <fstream>
#if USE_AZURE == 1
#include <iostream>
#include <azure/storage/blobs.hpp>
using namespace Azure::Storage::Blobs;
#endif

class Logger {
public:
    void init(const char * log_file_name);
    void release();

    void enqueueRecord(LogRecord* record);
    void processRecord(uint64_t thd_id);
    void flushBuffer(uint64_t thd_id);
    void writeToBuffer(uint64_t thd_id, LogRecord * record);
    void buf_to_log(std::string buf);
    #if USE_AZURE == 1
    void init_blob();
    void init_azure();
    bool copy_to_buf(uint64_t thd_id, LogRecord * record);
    void reset_buf();
    void updateBuffer(uint64_t thd_id);
    #endif
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

    #if USE_AZURE == 1
    const char* connectionString;
    BlobContainerClient containerClient;
    char buf[1000];
    int buf_p;
    #endif
};


#endif
