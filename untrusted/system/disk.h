#include "common/config.h"
#include "common/index_hash.h"
#include "common/index_btree.h"

#include <grpcpp/grpcpp.h>
#include "storage.grpc.pb.h"

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(kvstore::Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    kvstore::HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    kvstore::HelloReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<kvstore::Greeter::Stub> stub_;
};


class PageLoaderClient {
 public:
  PageLoaderClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(kvstore::PageLoader::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string LoadPage(const std::string& page_id) {
    // Data we are sending to the server.
    kvstore::GetPageRequest request;
    request.set_page_id(page_id);

    // Container for the data we expect from the server.
    kvstore::GetPageReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->GetPage(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      int size = reply.dataitem_size();
      for (int i = 0; i < size; i++) {
        kvstore::Item item = reply.dataitem(i);
        // TODO: orgainize to a page
      }
      return "Get Page success";
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  void ShutdownServer() {
    kvstore::ShutdownRequest request;
    request.set_signal("1");
    // Container for the data we expect from the server.
    kvstore::ShutdownReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->ShutdownServer(&context, request, &reply);

    if (status.ok()) {
      std::cout << "shutdown returns ok" << std::endl;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
            << std::endl;
    }
  }

  std::string SendLogBatch(kvstore::LogReplayRequest &request) {
    // Data we are sending to the server.

    // Container for the data we expect from the server.
    kvstore::LogReplayReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->LogReplay(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      // std::cout << "replay num:" << reply.numreplay() << std::endl;
      return "Log replay success";
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

private:
  std::unique_ptr<kvstore::PageLoader::Stub> stub_;
};


class RemoteStorage {

public:
    // storage the value of c to key <iname, part_id, bkt_idx>
    void flush_out_disk(std::string iname, int part_id, uint64_t pg_id, PAGE *c) {
    }

    PAGE* load_page_disk(std::string iname, int part_id, uint64_t pg_id) {

        // Instantiate the client. It requires a channel, out of which the actual RPCs
        // are created. This channel models a connection to an endpoint (in this case,
        // localhost at port 50051). We indicate that the channel isn't authenticated
        // (use of InsecureChannelCredentials()).
        PageLoaderClient pageloader(grpc::CreateChannel(
            RPC_SERVER, grpc::InsecureChannelCredentials()));
        std::string page_id("world");
        std::string reply = pageloader.LoadPage(page_id);

        std::cout << reply << std::endl;
        // std::cout << "kvstore::Greeter received: " << reply << std::endl;

        return nullptr;
    }

    void shutdown_server() {
      PageLoaderClient pageloader(grpc::CreateChannel(
            RPC_SERVER, grpc::InsecureChannelCredentials()));
        pageloader.ShutdownServer();
    }

    void send_log(kvstore::LogReplayRequest &request) {
      PageLoaderClient pageloader(grpc::CreateChannel(
            RPC_SERVER, grpc::InsecureChannelCredentials()));
        pageloader.SendLogBatch(request);
    }
};




