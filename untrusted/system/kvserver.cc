#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "global.h"
#include "kvserver.h"
#include "rocksdb/db.h"
#include <grpcpp/grpcpp.h>
#include "storage.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kvstore::HelloRequest;
using kvstore::HelloReply;
using kvstore::Greeter;
using kvstore::GetPageRequest;
using kvstore::GetPageReply;
using kvstore::Item;
using kvstore::PageLoader;
using kvstore::ShutdownRequest;
using kvstore::ShutdownReply;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
    Status SayHello(ServerContext* context, const HelloRequest* request,
                    HelloReply* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return Status::OK;
    }
};

class PageLoaderServiceImpl final : public PageLoader::Service {
    Status GetPage(ServerContext* context, const GetPageRequest* request,
                    GetPageReply* reply) override {
        std::string page_id = request->page_id();
        // read from rocksdb
        // std::map<std::string> items;
        auto f_proc_entry = [this, reply](const rocksdb::Iterator * it) {
            Item* item = reply->add_dataitem();
            item->set_key(it->key().data());
            item->set_value(it->value().data());
            // items.emplace_back(it->value().data());
        };
        eng->DBPrefixScan(page_id, f_proc_entry);
        return Status::OK;
    }
    Status ShutdownServer(ServerContext* context, const ShutdownRequest* request,
                ShutdownReply* reply) override {
        reply->set_signalret("1");
        delete server;
        // ExitThread(0);
        return Status::OK;
    }

};

void kvserver::RunServer() {
  std::string server_address("0.0.0.0:50051");
//   GreeterServiceImpl service;
  PageLoaderServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  // std::unique_ptr<Server> server(builder.BuildAndStart());
  server_ = builder.BuildAndStart();

  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server_->Wait();
}