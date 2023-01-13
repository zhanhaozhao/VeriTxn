#ifndef DBDISK_H_
#define DBDISK_H_

#include "config.h"
#include "global.h"

// #include <grpcpp/grpcpp.h>
// #include <grpcpp/channel.h>
// #include "storage.grpc.pb.h"

// class GreeterClient {
//  public:
//   GreeterClient(std::shared_ptr<grpc::Channel> channel)
//       : stub_(kvstore::Greeter::NewStub(channel)) {}

//   // Assembles the client's payload, sends it and presents the response back
//   // from the server.
//   std::string SayHello(const std::string& user);

//  private:
//   std::unique_ptr<kvstore::Greeter::Stub> stub_;
// };


// class PageLoaderClient {
//  public:
//   PageLoaderClient(std::shared_ptr<grpc::Channel> channel)
//       : stub_(kvstore::PageLoader::NewStub(channel)) {}

//   // Assembles the client's payload, sends it and presents the response back
//   // from the server.
//   std::string LoadPage(const std::string& page_id) {
//     // Data we are sending to the server.
//     kvstore::GetPageRequest request;
//     request.set_page_id(page_id);

//     // Container for the data we expect from the server.
//     kvstore::GetPageReply reply;
//     // Context for the client. It could be used to convey extra information to
//     // the server and/or tweak certain RPC behaviors.
//     grpc::ClientContext context;

//     // The actual RPC.
//     grpc::Status status = stub_->GetPage(&context, request, &reply);

//     // Act upon its status.
//     if (status.ok()) {
//       int size = reply.dataitem_size();
//       std::string res = "";
//       for (int i = 0; i < size; i++) {
//         kvstore::Item item = reply.dataitem(i);
//         // TODO: orgainize to a page
// //        res.append("[" + item.key() + ":" + item.value() + "]");
//       }
//       return res;
//     } else {
//       std::cout << status.error_code() << ": " << status.error_message()
//                 << std::endl;
//       return "RPC failed";
//     }
//   }

//   void ShutdownServer() {
//     kvstore::ShutdownRequest request;
//     request.set_signal("1");
//     // Container for the data we expect from the server.
//     kvstore::ShutdownReply reply;
//     // Context for the client. It could be used to convey extra information to
//     // the server and/or tweak certain RPC behaviors.
//     grpc::ClientContext context;

//     // The actual RPC.
//     grpc::Status status = stub_->ShutdownServer(&context, request, &reply);

//     if (status.ok()) {
//       std::cout << "shutdown returns ok" << std::endl;
//     } else {
//       std::cout << status.error_code() << ": " << status.error_message()
//             << std::endl;
//     }
//   }

//   std::string SendLogBatch(kvstore::LogReplayRequest &request) {
//     // Data we are sending to the server.

//     // Container for the data we expect from the server.
//     kvstore::LogReplayReply reply;
//     // Context for the client. It could be used to convey extra information to
//     // the server and/or tweak certain RPC behaviors.
//     grpc::ClientContext context;

//     // The actual RPC.
//     grpc::Status status = stub_->LogReplay(&context, request, &reply);

//     // Act upon its status.
//     if (status.ok()) {
//       // std::cout << "replay num:" << reply.numreplay() << std::endl;
//       return "Log replay success";
//     } else {
//       std::cout << status.error_code() << ": " << status.error_message()
//                 << std::endl;
//       return "RPC failed";
//     }
//   }

// private:
//   std::unique_ptr<kvstore::PageLoader::Stub> stub_;
// };


class RemoteStorage {

public:
    RemoteStorage();

    // storage the value of c to key <iname, part_id, bkt_idx>
    // void flush_out_disk(std::string iname, int part_id, uint64_t pg_id, PAGE *c) {
    //     assert(false);  // the data cache does not need to flush out data since updates are sent to disks via logs.
    // }

    void load_page_disk(std::string iname, int part_id, uint64_t pg_id);
    void send_log(char * log_entry, uint32_t size);
    void vaccum(uint64_t recover_ts);

    // void shutdown_server() {
    //   pageloader->ShutdownServer();
    // }

    // void send_log(char * log_entry, uint32_t size) {
    //   kvstore::LogEntry * entry = request->add_entry();
    //   entry->set_data(std::string(log_entry, size));
    //   entry->set_size(size);
    //   batch_num ++;
    //   if (batch_num >= LOG_BATCH_SIZE) {
    //     // request.Clear();
    //     pageloader->SendLogBatch(*request);
    //     delete request;
    //     request = new kvstore::LogReplayRequest();
    //     batch_num = 0;
    //     assert(request->entry_size() == 0);
    //   }
    // }

private:
    // std::shared_ptr<grpc::Channel> _channel;
    // PageLoaderClient *pageloader;
    // kvstore::LogReplayRequest * request;
    uint32_t    batch_num;
    uint32_t    batch_size;
    char *      log_buffer;
    int         sockfd;
    pthread_mutex_t mtx;
};



#endif
