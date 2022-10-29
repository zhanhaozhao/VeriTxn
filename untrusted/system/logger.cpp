#include "logger.h"
// #include "work_queue.h"
#include "message.h"
#include "helper.h"
// #include "mem_alloc.h"
#if USE_AZURE == 1
#include <iostream>
#include <azure/storage/blobs.hpp>
using namespace Azure::Storage::Blobs;

void Logger::init_blob() {
    std::string containerName = "DBx1000-zzhtest";

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

void Logger::init(const char * log_file_name) {
    #if USE_AZURE == 1
        init_azure();
    #else
        this->log_file_name = log_file_name;
        log_file.open(log_file_name, std::ios::out | std::ios::app | std::ios::binary);
        assert(log_file.is_open());
    #endif
    pthread_mutex_init(&mtx,NULL);
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

