#ifndef KVSERVER_H_
#define KVSERVER_H_

#include <iostream>
#include <memory>
#include <string>

#include "rpc_thread.h"

#include <grpcpp/grpcpp.h>
// #include "storage.grpc.pb.h"
class RPCThread;

class kvserver {
public:
    // kvserver();
    ~kvserver() {
        server_->Shutdown();
    }
    void RunServer(RPCThread * thread);
    std::unique_ptr<grpc::Server> server_;
    // RPCThread * thread_;
};

#endif