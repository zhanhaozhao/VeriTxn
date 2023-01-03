
#ifndef LOGGER_H
#define LOGGER_H

// #include "global.h"
#include "helper.h"
#include "log.h"
// #include "concurrentqueue.h"
#include <set>
#include <queue>
#include <fstream>

// #include <grpcpp/grpcpp.h>
// #include "storage.grpc.pb.h"

#if USE_AZURE == 1
#include <iostream>
#include <azure/storage/blobs.hpp>
using namespace Azure::Storage::Blobs;
#endif

class Logger {
public:
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

public:
	Logger(); // uint32_t logger_id
	~Logger();

	////////////////////////////	
	// forward processing
	////////////////////////////

public:
    void init(std::string log_file_name);
    uint64_t logTxn(char * log_entry, uint32_t size, uint64_t epoch, bool sync, uint64_t thd_id);
    uint64_t serialLogTxn(std::string buf, int size, uint64_t thd_id);

	// called by logging thread. 
	// return value: bytes flushed to disk
	uint64_t tryFlush();
	uint64_t get_persistent_lsn() {
		//assert(*_persistent_lsn < 2e9);
		return *_persistent_lsn;
	}

	void flush(uint64_t start_lsn, uint64_t end_lsn);
	// _lsn and _persistent_lsn must be stored in separate cachelines to avoid false sharing;
	// Because they are updated by different threads.
	uint64_t padding[8];
	volatile uint64_t _lsn[16];      		// log_tail. 
	//volatile uint64_t padding[8];
	//volatile uint64_t _lsn[8];
	volatile uint64_t _persistent_lsn[16];
	//volatile uint64_t  ** _filled_lsn; //[16];
	volatile uint64_t  * _filled_lsn[128]; //[16];

    volatile uint64_t  * _allocate_lsn[128];

	uint64_t padding2[8];

	uint64_t * _last_flush_time; 
	uint64_t _flush_interval;

	uint64_t _log_buffer_size;
    uint64_t _fdatasize;

	////////////////////////////	
	// recovery 
	////////////////////////////	

public:
	// bytes read from the log
	uint64_t	tryReadLog();
	bool 		iseof() { return _eof; }
	uint64_t	get_next_log_entry_non_atom(char * &entry); //, uint32_t &size);
	volatile uint64_t ** rasterizedLSN __attribute__((aligned(64)));
	uint64_t padding_rasterizedLSN[7];


 	uint64_t 	get_next_log_entry(char * &entry, uint32_t &callback_size);
	 uint64_t 	get_next_log_batch(char * &entry, uint32_t &num);
	uint32_t 	get_next_log_chunk(char * &chunk, uint64_t &size, uint64_t &base_lsn);
	void 		return_log_chunk(char * buffer, uint32_t chunk_num);
	void 		set_gc_lsn(uint64_t lsn, uint64_t thd_id);
	volatile bool     _eof __attribute__((aligned(64)));
private:
	volatile uint64_t * _disk_lsn; // __attribute__((aligned(64)));
	volatile uint64_t * _next_lsn; // __attribute__((aligned(64)));;
	uint64_t next_lsn;// __attribute__((aligned(64)));
	uint64_t disk_lsn;// __attribute__((aligned(64)));
	uint64_t padding_nlsn[6];
	volatile uint64_t ** _gc_lsn __attribute__((aligned(64)));

public:
	// log_tail for each buffer.
	//uint64_t ** 		_lsns;

	char * 				_buffer;		// circular buffer to store unflushed data.
	//uint32_t  			_curr_file_id;
		
	uint64_t * 			_starting_lsns;
	uint64_t 			_file_size;
	uint32_t 			_num_chunks;
	volatile uint32_t  	*_next_chunk;
	uint32_t 			_chunk_size;
	pthread_mutex_t * 	_mutex;
	
	std::string 				_file_name;
	int 				_fd; 
	int 				_fd_data; 
	//fstream * 			_file;
	uint32_t 			_logger_id;


};


#endif
