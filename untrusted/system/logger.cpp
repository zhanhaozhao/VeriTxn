#include "logger.h"
// #include "work_queue.h"
#include "message.h"
#include "helper.h"
// #include "mem_alloc.h"

#include <fcntl.h>
#include <inttypes.h>
#include "mem_helper.h"

#if USE_AZURE == 1
#include <iostream>
#include <azure/storage/blobs.hpp>
using namespace Azure::Storage::Blobs;

void Logger::init_blob() {
    std::string containerName = "DBx1000-test";

    // Initialize a new instance of BlobContainerClient
    containerClient
        = BlobContainerClient::CreateFromConnectionString(connectionString, containerName);

    // Create the container. This will do nothing if the container already exists.
    std::cout << "Creating container: " << containerName << std::endl;
    containerClient.CreateIfNotExists();
}

void Logger::init_azure() { 
    // Retrieve the connection string for use with the application. The storage
    // connection string is stored in an environment variable on the machine
    // running the application called AZURE_STORAGE_CONNECTION_STRING.
    // Note that _MSC_VER is set when using MSVC compiler.
    static const char* AZURE_STORAGE_CONNECTION_STRING = "DefaultEndpointsProtocol=https;AccountName=cs110032001d5074a52;AccountKey=dCSKtGeKgwIEjJZ9W2DumGWcEtv5T2YdkJVDKxuvasdTqCIq6//GS5tTWH4cW7ge9l8Am6jjU3yO+ASt28o0Ww==;EndpointSuffix=core.windows.net";
#if !defined(_MSC_VER)
    connectionString = std::getenv(AZURE_STORAGE_CONNECTION_STRING);
#else
    // Use getenv_s for MSVC
    size_t requiredSize;
    getenv_s(&requiredSize, NULL, NULL, AZURE_STORAGE_CONNECTION_STRING);
    if (requiredSize == 0) {
        throw std::runtime_error("missing connection string from env.");
    }
    std::vector<char> value(requiredSize);
    getenv_s(&requiredSize, value.data(), value.size(), AZURE_STORAGE_CONNECTION_STRING);
    std::string connectionStringStr = std::string(value.begin(), value.end());
    const char* connectionString = connectionStringStr.c_str();
#endif  
    reset_buf();
}

void Logger::reset_buf() {
    memset(buf, 0, 1000);
    buf_p = 0;
}

bool Logger::copy_to_buf(uint64_t thd_id, LogRecord * record) {
    // DEBUG("Buffer Write\n");
    //memcpy(aries_log_buffer + offset, data, size);
    //aries_write_offset += size;
    int potential_size = buf_p + 
                         sizeof(record->rcd.checksum) +
                         sizeof(record->rcd.lsn) +
                         sizeof(record->rcd.type) +
                         sizeof(record->rcd.iud) +
                         sizeof(record->rcd.txn_id) +
                         sizeof(record->rcd.table_id) +
                         sizeof(record->rcd.key);
    if (potential_size >= 1000) return false;           
    uint64_t starttime = get_sys_clock();
    
    COPY_BUF(buf,record->rcd.checksum,buf_p);
    COPY_BUF(buf,record->rcd.lsn,buf_p);
    COPY_BUF(buf,record->rcd.type,buf_p);
    COPY_BUF(buf,record->rcd.iud,buf_p);
    COPY_BUF(buf,record->rcd.txn_id,buf_p);
    COPY_BUF(buf,record->rcd.table_id,buf_p);
    COPY_BUF(buf,record->rcd.key,buf_p);
    return true;
}

void Logger::updateBuffer(uint64_t thd_id, LogRecord * record) {
    std::string blobName = "txn" + record->rcd.txn_id + "," + record->rcd.lsn;
    // uint8_t blobContent[] = ;
    uint8_t *blobContent = new uint8_t[buf_p];
    memcpy(blobContent, buf, buf_p);
    reset_buf();
    // Create the block blob client
    BlockBlobClient blobClient = containerClient.GetBlockBlobClient(blobName);

    // Upload the blob
    // std::cout << "Uploading blob: " << blobName << std::endl;
    blobClient.UploadFrom(blobContent, sizeof(blobContent));
}

#else
#include <fstream>
#endif

void Logger::init(std::string log_file_name) {
    // #if USE_AZURE == 1
    //     init_azure();
    // #else
    //     this->log_file_name = log_file_name;
    //     log_file.open(log_file_name, std::ios::out | std::ios::app | std::ios::binary);
    //     assert(log_file.is_open());
    // #endif
    pthread_mutex_init(&mtx, NULL);

    _log_buffer_size = g_log_buffer_size;
	
    if (g_log_recover) {

        // LOGREPLAY
        _disk_lsn = (uint64_t *) malloc(sizeof(uint64_t));
        *_disk_lsn = 0;
        _next_lsn = (uint64_t *) malloc(sizeof(uint64_t));
        *_next_lsn = 0;

        _gc_lsn = new uint64_t volatile * [g_thread_cnt];
        for (uint32_t i = 0; i < g_thread_cnt; i++) {
            _gc_lsn[i] = (uint64_t *) malloc(sizeof(uint64_t));
            *_gc_lsn[i] = 0;
        }
        _eof = false;

        _buffer = (char *) malloc(_log_buffer_size + g_max_log_entry_size);
        std::cout << "Replay Log buffer size " << _log_buffer_size << std::endl;

	} else {
		
        *_lsn = 0;
        *_persistent_lsn = 0;
        
        for (uint32_t i = 0; i < g_thread_cnt; i++) {
            _filled_lsn[i] = (uint64_t *) malloc(sizeof(uint64_t)); //logger_id
            *_filled_lsn[i] = 0; 
        }

        for (uint32_t i = 0; i < g_thread_cnt; i++) {
            _allocate_lsn[i] = (uint64_t *) malloc(sizeof(uint64_t));
            *_allocate_lsn[i] = (uint64_t) -1;
        }

	}

	// if(g_flush_interval==0)
	_flush_interval = UINT64_MAX; // flush interval turned off
	// else
	// 	_flush_interval = g_flush_interval; // in ns  
	_last_flush_time = (uint64_t *) malloc(sizeof(uint64_t)); //logger_id
	COMPILER_BARRIER
	*_last_flush_time = get_sys_clock();

	// _buffer = (char *) numa_alloc_onnode(_log_buffer_size + g_max_log_entry_size, (logger_id % g_num_logger) % NUMA_NODE_NUM);
    _buffer = (char *) malloc(_log_buffer_size + g_max_log_entry_size);

    std::cout << "Log buffer size " << _log_buffer_size << std::endl;
	assert(_buffer != 0);


    if (g_log_recover) {
        std::string path = log_file_name + ".0";
        // if(g_ramdisk)
        //     _fd_data = open(path.c_str(), O_RDONLY);
        // else
        _fd_data = open(path.c_str(), O_DIRECT | O_RDONLY);
        uint64_t fdatasize = lseek(_fd_data, 0, SEEK_END);
        std::cout << "data size " << fdatasize << std::endl;
        _fdatasize = fdatasize;

        lseek(_fd_data, 0, SEEK_SET);
        assert(_fd != -1);
    //if (g_log_recover) {
        int _fd = open(log_file_name.c_str(), O_RDONLY);
        assert(_fd != -1);
        uint32_t fsize = lseek(_fd, 0, SEEK_END);
        lseek(_fd, 0, SEEK_SET);
        _num_chunks = fsize / sizeof(uint64_t);
        //_starting_lsns = new uint64_t [_num_chunks];
        _starting_lsns = (uint64_t*) malloc(_num_chunks * sizeof(uint64_t));
        uint32_t size = read(_fd, _starting_lsns, fsize);
        assert(size == fsize);
        _chunk_size = 0;
        for (uint32_t i = 0; i < _num_chunks; i ++) {
            if (_starting_lsns[i] >= fdatasize)
            {
                _num_chunks = i;
                break;
            }
            uint64_t fstart = _starting_lsns[i] / 512 * 512; 
            uint64_t fend = _starting_lsns[i+1];
            if (fend % 512 != 0)
                fend = fend / 512 * 512 + 512; 
            if (fend - fstart > _chunk_size)
                _chunk_size = fend - fstart;
            printf("_starting_lsns[%d] = %" PRIu64 "\n", i, _starting_lsns[i]);
        }
        
        printf("Chunk Number adjusted to %d\n", _num_chunks);

        _chunk_size = _chunk_size / 512 * 512 + 1024;
        close(_fd);
        _next_chunk = (uint32_t*) malloc(sizeof(uint32_t));
        *_next_chunk = 0; //_num_chunks - 1; //_num_chunks-1; // start from end
        _file_size = fsize;
		
	} else {

        std::string name = log_file_name;
        // for parallel logging. The metafile format.
        //  | file_id | start_lsn | * num_of_log_files
        _fd = open(name.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0664);
        
        assert(*_lsn  == 0);
        uint32_t bytes = write(_fd, (uint64_t*)_lsn, sizeof(uint64_t));
        assert(bytes == sizeof(uint64_t));
        fsync(_fd);
        
        assert(_fd != -1);

        name = name + ".0"; // + to_string(_curr_file_id);

        _fd_data = open(name.c_str(), O_DIRECT | O_TRUNC | O_WRONLY | O_CREAT, 0664);
        assert(_fd_data != -1);

    }


}

void Logger::release() { log_file.close(); }

void Logger::buf_to_log(std::string buf) {
    int buf_p = 0;
    while (buf_p < buf.size()) {
        LogRecord * record = new LogRecord;
    
        COPY_VAL(record->rcd.checksum,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.lsn,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.type,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.iud,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.txn_id,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.table_id,buf.c_str(),buf_p);
        COPY_VAL(record->rcd.key,buf.c_str(),buf_p);
        enqueueRecord(record);
    }

}

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
        #if USE_AZURE == 1
            if(record->rcd.iud == L_NOTIFY) {
                updateBuffer(thd_id, record);
            }
            bool full = copy_to_buf(thd_id,record);
            if (full) {
                updateBuffer(thd_id, record);;
            }
            // std::string blobName = "txn" + record->rcd.txn_id;
        #else
            uint64_t starttime = get_sys_clock();
            // DEBUG("Dequeue Log Record %ld\n",record->rcd.txn_id);
            if(record->rcd.iud == L_NOTIFY) {
                flushBuffer(thd_id);
                // work_queue.enqueue(thd_id,Message::create_message(record->rcd.txn_id,LOG_FLUSHED),false);
            }
            writeToBuffer(thd_id,record);
            log_buf_cnt++;
            free(record);
        // INC_STATS(thd_id,log_process_time,get_sys_clock() - starttime);
        #endif
    }
}


void Logger::writeToBuffer(uint64_t thd_id, LogRecord * record) {
  // DEBUG("Buffer Write\n");
  //memcpy(aries_log_buffer + offset, data, size);
  //aries_write_offset += size;
  uint64_t starttime = get_sys_clock();

  WRITE_VAL(log_file,record->rcd.checksum);
  WRITE_VAL(log_file,record->rcd.lsn);
  WRITE_VAL(log_file,record->rcd.type);
  WRITE_VAL(log_file,record->rcd.iud);
  WRITE_VAL(log_file,record->rcd.txn_id);
  WRITE_VAL(log_file,record->rcd.table_id);
  WRITE_VAL(log_file,record->rcd.key);
}


void Logger::flushBuffer(uint64_t thd_id) {
  uint64_t starttime = get_sys_clock();
  log_file.flush();
  last_flush = get_sys_clock();
  log_buf_cnt = 0;
}



Logger::Logger() // uint32_t logger_id
	// : _logger_id (logger_id)
{}


Logger::~Logger()
{
// #if LOG_RAM_DISK
// 	if (!g_log_recover && !g_no_flush)
// 		_disk->flush(*_lsn);
// 	delete _disk;
// #else 
	if (!g_log_recover) {

        printf("Destructor %d. flush size=%" PRIu64 " (%" PRIu64 " to %" PRIu64 ")\n", _logger_id, (*_lsn) / 512 * 512 - *_persistent_lsn, 
            *_persistent_lsn, (*_lsn) / 512 * 512);
        uint64_t end_lsn = (*_lsn) / 512 * 512;
        uint64_t start_lsn = *_persistent_lsn;
        if(end_lsn > start_lsn)
            flush(start_lsn, end_lsn);
        // INC_FLOAT_STATS_V0(log_bytes, end_lsn - start_lsn);
            
        uint32_t bytes = write(_fd, &end_lsn, sizeof(uint64_t));
        //uint32_t bytes = write(_fd, _lsn, sizeof(uint64_t));
        assert(bytes == sizeof(uint64_t));
        fsync(_fd);

        close(_fd);
        close(_fd_data);
	}
	
	//_mm_free(_buffer); // because this could be very big.
	// numa_free((void*)_buffer, _log_buffer_size + g_max_log_entry_size);
    free(_buffer);
// #endif
}

uint64_t
Logger::logTxn(char * log_entry, uint32_t size, uint64_t epoch, bool sync, uint64_t thd_id)
{
	// called by serial logging AND parallel command logging 
	// The log is stored to the in memory cirular buffer
    // if the log buffer is full, wait for it to flush.  
		
	//assert( *(uint32_t*)log_entry == 0xbeef);

	//printf("TPCC log size: %d\n", size);
	//COMPILER_BARRIER;
	// uint64_t starttime = get_sys_clock();
	if (*_lsn + size >= 
		*_persistent_lsn + _log_buffer_size - g_max_log_entry_size * g_thread_cnt / g_num_logger) 
	{
		//printf("[%" PRIu64 "] txn aborted, persistent_lsn=%" PRIu64 "\n", GET_THD_ID, *_persistent_lsn);
		//assert(false);
		return -1; 
	}
	uint32_t size_aligned = size % 64 == 0 ? size: size + 64 - size % 64;
	//INC_INT_STATS(time_debug6, get_sys_clock() - starttime);

	if(log_entry[0] != 0x7f)
		*_allocate_lsn[thd_id] = *_lsn; 
	// assumingly this will be no less than the _filled_lsn

	COMPILER_BARRIER
	uint64_t lsn;
	if(sync)
  		lsn = ATOM_FETCH_ADD(*_lsn, size_aligned);	
	else
	{
		// assert(LOG_ALGORITHM == LOG_SERIAL R); // only serial logging puts this code in a critical section.	
		lsn = *_lsn;

		*_lsn += size_aligned;
	}
	
	if ((lsn / _log_buffer_size < (lsn + size) / _log_buffer_size))	{
		// reaching the end of the circular buffer, write in two steps 
		uint32_t tail_size = _log_buffer_size - lsn % _log_buffer_size; 
		memcpy(_buffer + lsn % _log_buffer_size, log_entry, tail_size);
		memcpy(_buffer, log_entry + tail_size, size - tail_size);
	} else {
		memcpy(_buffer + lsn % _log_buffer_size, log_entry, size);
	}

    // send log
    remotestorage->send_log(log_entry, size);

	COMPILER_BARRIER
	// INC_INT_STATS(time_insideSLT1, get_sys_clock() - starttime);

	*(_filled_lsn[thd_id]) = lsn + size_aligned; 

	// INC_INT_STATS(time_insideSLT2, get_sys_clock() - starttime);
  	return lsn + size; // or it could be lsn+size_aligned-1
}

uint64_t Logger::serialLogTxn(std::string buf, int entry_size, uint64_t thd_id) {
    
    uint64_t newlsn;
    char* log_entry = (char*) buf.data();

#if CC_ALG == SILO
	
	for(;;)
	{
		newlsn = logTxn(log_entry, entry_size, 0, true, thd_id); // need to sync
		if(newlsn < UINT64_MAX)
			break;
		
		PAUSE 
	}
	
#else
	for(;;)
	{
		#if CC_ALG == NO_WAIT
		newlsn = logTxn(log_entry, entry_size, 0, true, thd_id);
		#else
		newlsn = logTxn(log_entry, entry_size, 0, false, thd_id);
		#endif
		if(newlsn < UINT64_MAX)
			break;
		
		PAUSE
	}

#endif
	return newlsn;

}

uint64_t
Logger::tryFlush() 
{
	// entries before ready_lsn can be flushed. 
	// if (g_no_flush) {
	// 	uint32_t size = *_lsn - *_persistent_lsn; //*_persistent_lsn - *_lsn;
	// 	*_persistent_lsn = *_lsn;
	// 	return size;
	// }
	uint64_t ready_lsn = *_lsn;

	
	COMPILER_BARRIER
	// #if SOLVE_LIVELOCK
// #if PARTITION_AWARE
// 	for (uint32_t i = 0; i < g_thread_cnt; i++) // because any worker could write to this log
// #else
	for (uint32_t i = _logger_id; i < g_thread_cnt; i+= g_num_logger)
// #endif	
	{
		uint64_t filledLSN = *_filled_lsn[i]; // the reading order matters
		COMPILER_BARRIER
		uint64_t allocateLSN = *_allocate_lsn[i];
		if (allocateLSN >= filledLSN && ready_lsn > allocateLSN) {
			ready_lsn = allocateLSN;
		}
	}
	// #else
// #if PARTITION_AWARE
	// for (uint32_t i = 0; i < g_thread_cnt; i++) // because any worker could write to this log
// #else
	// for (uint32_t i = _logger_id; i < g_thread_cnt; i+= g_num_logger)
// #endif
		// if (ready_lsn > *_filled_lsn[i]) {
			// ready_lsn = *_filled_lsn[i];
		// }
	// #endif

	if (get_sys_clock() - *_last_flush_time < _flush_interval &&
	    ready_lsn - *_persistent_lsn < g_flush_blocksize) 
	{	
		return 0;
	}
	// if(ready_lsn - *_persistent_lsn < g_flush_blocksize)
	// {
	// 	INC_INT_STATS(int_flush_time_interval, 1); // caused by time larger than _flush_interval
	// }
	// else
	// {
	// 	INC_INT_STATS(int_flush_half_full, 1); // caused by half full buffer
	// }

	assert(ready_lsn >= *_persistent_lsn);
	// timeout or buffer full enough.
	*_last_flush_time = get_sys_clock();
	
	uint64_t start_lsn = *_persistent_lsn;
	uint64_t end_lsn = ready_lsn - ready_lsn % 512; // round to smaller 512x

	if(end_lsn - start_lsn > g_flush_blocksize)
		end_lsn = start_lsn + g_flush_blocksize;


	flush(start_lsn, end_lsn);

	/*******************************/
	COMPILER_BARRIER
	//printf("[%" PRIu64 "] update persistent lsn from %" PRIu64 " to %" PRIu64 ", ready=%" PRIu64 "\n", GET_THD_ID, *_persistent_lsn, end_lsn, ready_lsn);
	*_persistent_lsn = end_lsn;

	uint32_t chunk_size = g_thread_cnt;// _log_buffer_size 
	if (end_lsn / chunk_size  > start_lsn / chunk_size ) {
		// write ready_lsn into the file.

		uint32_t bytes = write(_fd, &ready_lsn, sizeof(ready_lsn)); // TODO: end_lsn??
		assert(bytes == sizeof(ready_lsn));
		fsync(_fd);
	}

	return end_lsn - start_lsn;
}

void 
Logger::flush(uint64_t start_lsn, uint64_t end_lsn)
{
	uint64_t starttime = get_sys_clock();

	if (start_lsn == end_lsn) return;
	assert(end_lsn - start_lsn < _log_buffer_size);
	
	assert(_fd_data != 1 && _fd_data != 0);
	uint32_t bytes;
	if (start_lsn / _log_buffer_size < end_lsn / _log_buffer_size) {
		// flush in two steps.
		uint32_t tail_size = _log_buffer_size - start_lsn % _log_buffer_size;
		bytes = write(_fd_data, _buffer + start_lsn % _log_buffer_size, tail_size); 
		assert(bytes == tail_size);
		bytes = write(_fd_data, _buffer, end_lsn % _log_buffer_size); 
		assert(bytes == end_lsn % _log_buffer_size);
	} else { 
		// here an error might occur that, the serial port (SATA) might be being used by another one.
		// where the error number would be 
		bytes = write(_fd_data, (void *)(_buffer + start_lsn % _log_buffer_size), end_lsn - start_lsn);
		// When using RAID0, sometimes only 2147479552 bytes are written

		M_ASSERT(bytes == end_lsn - start_lsn, "bytes=%d, planned=%" PRIu64 ", errno=%d, _fd=%d, end_lsn=%" PRIu64 ", start_lsn=%" PRIu64 ", data=%" PRIu64 "\n", 
			bytes, end_lsn - start_lsn, errno, _fd_data, end_lsn, start_lsn, (uint64_t)(_buffer));
		//printf("start_lsn = %ld, end_lsn = %ld\n", start_lsn, end_lsn);
		//assert(*(uint32_t*)(_buffer + start_lsn % _log_buffer_size) == 0xbeef);
	}
	// INC_INT_STATS(int_debug2, bytes);
	// INC_INT_STATS(int_debug3, 1);
	fsync(_fd_data); // sync the data
	// INC_INT_STATS_V0(time_io, get_sys_clock() - starttime); // actual time in flush.
//#endif
}

uint64_t 
Logger::tryReadLog()
{
	uint64_t bytes, start_lsn_moded, end_lsn_moded;
	bytes = 0;
// #if ASYNC_IO
// 	if(AIOworking){
// 		// short-cut tryReadLog function if the previous is till working
// 		if(aio_error64(&cb) == EINPROGRESS)
// 			return 0;
// 		bytes = aio_return64(&cb);
// 		//cout << GET_THD_ID << " Read bytes:" << bytes << endl;
// 		if(bytes < lastSize)
// 			_eof = true;
// 		fileOffset += bytes;
// 		start_lsn_moded = lastStartLSN % _log_buffer_size;
// 		end_lsn_moded = (lastStartLSN + bytes) % _log_buffer_size;
// 		assert(end_lsn_moded == 0 || end_lsn_moded >= start_lsn_moded); // bytes could be 0
// 		if(start_lsn_moded < g_max_log_entry_size)
// 		{
// 			if(end_lsn_moded > 0 && end_lsn_moded < g_max_log_entry_size)
// 				memcpy(_buffer + start_lsn_moded + _log_buffer_size, _buffer + start_lsn_moded, end_lsn_moded - start_lsn_moded);
// 			else
// 				memcpy(_buffer + start_lsn_moded + _log_buffer_size, _buffer + start_lsn_moded, g_max_log_entry_size - start_lsn_moded);
// 		}
// 		AIOworking = false;

// 		*_disk_lsn = lastStartLSN + bytes;

// 		INC_INT_STATS(int_flush_half_full, 1);
// 		INC_INT_STATS(log_data, bytes);
// 		INC_INT_STATS_V0(time_io, get_sys_clock() - lastTime);
// 	}
// #endif

	uint64_t starttime = get_sys_clock();

	uint64_t gc_lsn = *_next_lsn;
	COMPILER_BARRIER
	for (uint32_t i = _logger_id; i < g_thread_cnt; i+=g_num_logger)
		if (gc_lsn > *_gc_lsn[i] && *_gc_lsn[i] != 0) {
		//if (i % g_num_logger == _logger_id && gc_lsn > *_gc_lsn[i] && *_gc_lsn[i] != 0) {
			gc_lsn = *_gc_lsn[i];
		}
	assert(gc_lsn <= *_disk_lsn);
	if (*_disk_lsn - gc_lsn > _log_buffer_size / 2) 
		return bytes;
	assert(*_disk_lsn >= gc_lsn);
	assert(*_disk_lsn % 512 == 0);
	uint64_t start_lsn = *_disk_lsn;
		
	gc_lsn -= gc_lsn % 512; 

	
	uint64_t budget = g_read_blocksize; //(uint64_t)(_log_buffer_size * g_recover_buffer_perc);
	uint64_t end_lsn = start_lsn + budget;
	if (end_lsn - gc_lsn >= _log_buffer_size)
		end_lsn = gc_lsn + _log_buffer_size - 512;
	
	if (end_lsn - start_lsn < budget / 2) // we need to control the amount of bytes every time
		return bytes;
	
	//uint64_t end_lsn = gc_lsn + (uint64_t)(_log_buffer_size * g_recover_buffer_perc);
	if (start_lsn == end_lsn) return bytes;
	assert(end_lsn - start_lsn <= _log_buffer_size);
	//uint32_t bytes;
	start_lsn_moded = start_lsn % _log_buffer_size;
	end_lsn_moded = end_lsn % _log_buffer_size;
	
// #if ASYNC_IO
// 	lastStartLSN = start_lsn;
// 	//memset(&cb, 0, sizeof(aiocb64));
// 	//cb.aio_fildes = _fd_data;
// 	cb.aio_buf = _buffer + start_lsn_moded;
// 	cb.aio_offset = fileOffset;
// 	if (end_lsn_moded > 0 && start_lsn_moded >= end_lsn_moded) {
// 		// flush in two steps.
// 		uint32_t tail_size = _log_buffer_size - start_lsn_moded;
// 		cb.aio_nbytes = tail_size;
// 		lastSize = tail_size;
// 		//bytes = read(_fd_data, _buffer + start_lsn_moded, tail_size);
// 	} else { 
// 		cb.aio_nbytes = end_lsn - start_lsn;
// 		lastSize = end_lsn - start_lsn;
// 	}
// 	//cout << GET_THD_ID << " aio read from " << start_lsn << " to " << end_lsn << "|" << lastSize << endl;
// 	lastTime = get_sys_clock();
// 	if(aio_read64(&cb) == -1)
// 	{
// 		assert(false); // Async request failure
// 	}
// 	AIOworking = true;
// #else
	if (end_lsn_moded > 0 && start_lsn_moded >= end_lsn_moded) {
		// flush in two steps.
		uint32_t tail_size = _log_buffer_size - start_lsn_moded;

		bytes = read(_fd_data, _buffer + start_lsn_moded, tail_size);
		if(start_lsn_moded < g_max_log_entry_size)
			memcpy(_buffer + start_lsn_moded + _log_buffer_size, _buffer + start_lsn_moded, g_max_log_entry_size - start_lsn_moded);
		if (bytes < tail_size) 
			_eof = true; 
		else {
			bytes += read(_fd_data, _buffer, end_lsn_moded);
            if (bytes < end_lsn  - start_lsn)
				_eof = true; 
			// fill in the dummy tail
			//cout << GET_THD_ID % g_num_logger << " Wrapping " << start_lsn << " " << end_lsn << " " << _log_buffer_size << endl;
			memcpy(_buffer+_log_buffer_size, _buffer, end_lsn_moded < g_max_log_entry_size ? end_lsn_moded : g_max_log_entry_size);
		}

	} else { 
		bytes = read(_fd_data, _buffer + start_lsn_moded, end_lsn - start_lsn);
		//M_ASSERT(bytes == end_lsn - start_lsn, "bytes=%d, planned=%" PRIu64 ", errno=%d, _fd=%d, end_lsn=%" PRIu64 ", start_lsn=%" PRIu64 ", data=%" PRIu64 "\n", 
		//	bytes, end_lsn - start_lsn, errno, _fd_data, end_lsn, start_lsn, (uint64_t)(_buffer));
        if (bytes < end_lsn - start_lsn) 
			_eof = true;
		if(start_lsn_moded < g_max_log_entry_size)
		{
			if(end_lsn_moded > 0)
				memcpy(_buffer + start_lsn_moded + _log_buffer_size, _buffer + start_lsn_moded, (end_lsn_moded < g_max_log_entry_size ? end_lsn_moded : g_max_log_entry_size) - start_lsn_moded);
			else
			{
				memcpy(_buffer + start_lsn_moded + _log_buffer_size, _buffer + start_lsn_moded, g_max_log_entry_size - start_lsn_moded);
			}
		}
	}
	end_lsn = start_lsn + bytes;
	
	COMPILER_BARRIER

    *_disk_lsn = end_lsn;

	// INC_INT_STATS(int_debug3, 1);
	// INC_INT_STATS(int_debug2, bytes);
	// INC_INT_STATS_V0(time_io, get_sys_clock() - starttime);
// #endif
    // INC_INT_STATS(int_debug1, 1); // how many time we initiate the AIO.
	// INC_INT_STATS(time_debug10, get_sys_clock() - starttime); // actual time in read.
	return bytes;
/*#else 
	assert(false);
	return 0;
#endif*/
}

uint64_t Logger::get_next_log_entry_non_atom(char * &entry) //, uint32_t &mysize)
{
	//uint64_t next_lsn;
	uint32_t size;
	uint64_t t1 = get_sys_clock();
	
	//next_lsn = *_next_lsn;
    
    // TODO. 
    // There is a werid bug: for the last block (512-bit) of the file, the data 
    // is corrupted? the assertion in txn.cpp : 449 would fail.
    // Right now, the hack to solve this bug:
    //  	do not read the last few blocks.
    uint32_t dead_tail = 0;// _eof? 2048 : 0;
    if (UNLIKELY(*_next_lsn + sizeof(uint32_t) * 2 >= *_disk_lsn - dead_tail)) {
        entry = NULL;
        // INC_INT_STATS(time_debug11, get_sys_clock() - t1);
        return -1;
    }
    // Assumption. 
    // Each log record has the following format
    //  | checksum (32 bit) | size (32 bit) | ...

    uint64_t size_offset = *_next_lsn % _log_buffer_size + sizeof(uint32_t);
    
    size = *(uint32_t*) (_buffer + size_offset);
    //mysize = size;
    
    // round to a cacheline size
    size = size % 64 == 0 ? size : size + 64 - size % 64;
    //INC_INT_STATS(time_debug6, get_sys_clock() - t2);
    if (UNLIKELY(*_next_lsn + size >= *_disk_lsn - dead_tail)) {
        entry = NULL;
        // INC_INT_STATS(time_debug12, get_sys_clock() - t1);
        return -1;
    }
    //INC_INT_STATS(int_debug5, 1);
	uint64_t tt2 = get_sys_clock();
	// INC_INT_STATS(time_debug_get_next, tt2 - t1);
	
	entry = _buffer + (*_next_lsn % _log_buffer_size);
		// TODO: assume file read is slower than processing txn,
		// a.k.a., the circular buffer will not be freshed.
	*_next_lsn = *_next_lsn + size;

	// INC_INT_STATS(int_debug_get_next, 1);	
	// INC_INT_STATS(time_recover5, get_sys_clock() - t1);
	return *_next_lsn - 1; // for maxLSN

}

void 
Logger::set_gc_lsn(uint64_t lsn, uint64_t thd_id)
{
	*_gc_lsn[thd_id] = lsn;
}