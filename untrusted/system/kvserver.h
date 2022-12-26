#ifndef KVSERVER_H_
#define KVSERVER_H_

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
// #include "storage.grpc.pb.h"

class kvserver {
public:
    // kvserver();
    // ~kvserver() = default;
    void RunServer();
    std::unique_ptr<grpc::Server> server;
};

#endif