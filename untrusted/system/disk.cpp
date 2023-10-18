#include "disk.h"
#include "../../common/config.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


RemoteStorage::RemoteStorage() {
    // _channel = grpc::CreateChannel(
            // RPC_SERVER, grpc::InsecureChannelCredentials());
    // pageloader =  new PageLoaderClient(_channel);
    // request = new kvstore::LogReplayRequest();
    batch_num = 0;
    batch_size = 0;

    log_buffer = (char *) malloc(MAX_LINE);

	struct sockaddr_in servaddr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, RPC_SERVER, &servaddr.sin_addr) < 0) {
		printf("inet_pton error for %s\n", RPC_SERVER);
		exit(1);
	}

	if( connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect error");
		exit(1);
	}

    pthread_mutex_init(&mtx, NULL);
    std::cout<< "Connecting to the storage suceess." << std::endl;

}

void RemoteStorage::load_page_disk(std::string iname, int part_id, uint64_t pg_id) {

    pthread_mutex_lock(&mtx);
    // std::cout<< "entering load disk" << std::endl;

    auto keys = new char [50];
    sprintf(keys, "%d_%lu_", part_id, pg_id);
    std::string page_id(keys);

    write(sockfd, page_id.data(), page_id.size());

    char * recvline = (char *) malloc(sizeof(long));

	if(read(sockfd, recvline, sizeof(long)) == 0) {
		perror("server terminated prematurely");
		exit(1);
	}
//    std::cout<< "received from client:" << atol(recvline) << std::endl;
    pthread_mutex_unlock(&mtx);
}

void RemoteStorage::send_log(char * log_entry, uint32_t size){

    // Format for a log message
	// | LOGS | entry_num:4 | (entry_size:4 | log_entry_data:?) * BATCH_SIZE

    pthread_mutex_lock(&mtx);
    // std::cout<< "entering send logs" << std::endl;

    if (batch_num == 0) { // init buffer prefix
        batch_size = 0;
        memset(log_buffer, 0, MAX_LINE);

        char * prefix = "LOGS"; 
        PACK_SIZE(log_buffer, prefix, strlen(prefix), batch_size);
        batch_size += sizeof(uint32_t);
        // batch_size += sizeof(uint32_t);
    }

    if (batch_num < LOG_BATCH_SIZE) {
        PACK(log_buffer, size, batch_size);
        PACK_SIZE(log_buffer, log_entry, size, batch_size);
        batch_num ++;
    } else {
        // std::string test_str = "log_entry";
        // memcpy(log_buffer + sizeof(uint32_t), &batch_size, sizeof(uint32_t));
        memcpy(log_buffer + sizeof(uint32_t), &batch_num, sizeof(uint32_t));
        write(sockfd, log_buffer, batch_size);
        // std::cout<< "batch size:" << batch_size << std::endl;

//        char * recvline = (char *) malloc(sizeof(uint32_t));
//        if(read(sockfd, recvline, sizeof(uint32_t)) == 0) {
//            perror("server terminated prematurely");
//		    exit(1);
//	    }
        // uint32_t process_count = 0;
        // uint32_t offset = 0;
        // UNPACK(&recvline, process_count, offset);
//        std::cout<< "received from client:" << atoi(recvline) << std::endl;
        batch_num = 0;
    }
    pthread_mutex_unlock(&mtx);

    //   kvstore::LogEntry * entry = request->add_entry();
    //   entry->set_data(std::string(log_entry, size));
    //   entry->set_size(size);

    //     // request.Clear();
    //     pageloader->SendLogBatch(*request);
    //     delete request;
    //     request = new kvstore::LogReplayRequest();
    //     assert(request->entry_size() == 0);

}

void RemoteStorage::vaccum(uint64_t recover_ts){

    // Format for a log message
    // | VACU | recover_ts:4

    pthread_mutex_lock(&mtx);

    char *msg = new char [100];
    memset(msg, 0, 100);
    uint64_t offset = 0;

    char * prefix = "VACU";
    PACK_SIZE(msg, prefix, strlen(prefix), offset);
    PACK(msg, recover_ts, offset);
    write(sockfd, msg, offset);
    pthread_mutex_unlock(&mtx);
}
