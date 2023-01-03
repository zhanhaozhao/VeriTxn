#include "disk.h"


std::string GreeterClient::SayHello(const std::string& user) {
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

RemoteStorage::RemoteStorage() {
    _channel = grpc::CreateChannel(
            RPC_SERVER, grpc::InsecureChannelCredentials());
    pageloader =  new PageLoaderClient(_channel);
    batch_num = 0;
    request = new kvstore::LogReplayRequest();
}

void RemoteStorage::load_page_disk(std::string iname, int part_id, uint64_t pg_id) {

    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    auto keys = new char [50];
    sprintf(keys, "%d_%lu_", part_id, pg_id);
    std::string page_id(keys);
    std::string reply = pageloader->LoadPage(page_id);
//        std::cout << reply << ' ' << page_id << std::endl;
    // std::cout << "kvstore::Greeter received: " << reply << std::endl;
    // return nullptr;
}
