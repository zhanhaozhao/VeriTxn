#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "global.h"
#include "global_struct.h"
#include "kvserver.h"
#include "rocksdb/db.h"
#include <grpcpp/grpcpp.h>
#include "storage.grpc.pb.h"
#include "re_ycsb_txn.h"

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
using kvstore::LogEntry;
using kvstore::LogReplayRequest;
using kvstore::LogReplayReply;

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
public:   
    PageLoaderServiceImpl (RPCThread * thread) {thread_ = thread;}

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

    Status LogReplay(ServerContext* context, const LogReplayRequest* request,
            LogReplayReply* reply) override {

        int size = request->entry_size();
        for (int i = 0; i < size; i++) {
            LogEntry entry = request->entry(i);
            RC rc = RCOK;
            re_txn_man * m_txn;

            uint64_t thd_id = thread_->get_thd_id();
            // uint64_t thd_id = 0;

            assert (glob_manager);
            switch (WORKLOAD) {
            case YCSB :
                // m_txn = (ycsb_txn_man *) aligned_alloc(64, sizeof(ycsb_txn_man));
                m_txn = (re_ycsb_txn_man *) malloc(sizeof(re_ycsb_txn_man));
                new(m_txn) re_ycsb_txn_man();
                break;
            // case TPCC :
            // 	// m_txn = (tpcc_txn_man *) aligned_alloc(64, sizeof(tpcc_txn_man));
            // 	m_txn = (re_tpcc_txn_man *) malloc(sizeof(re_tpcc_txn_man));
            // 	new(m_txn) re_tpcc_re_txn_man();
            // 	break;
            default:
                assert(false);
            }

            m_txn->init(thread_, thread_->_wl, thd_id);
            glob_manager->set_txn_man(m_txn);
            assert (m_txn);

            m_txn->set_txn_id(thd_id + glob_manager->get_thd_txn_id(thd_id) * g_thread_cnt);
            glob_manager->set_thd_txn_id(thd_id);

            //if (get_thd_id() == 0)
            COMPILER_BARRIER
            m_txn->recover_txn((char *)entry.data().data());
            COMPILER_BARRIER

        }
        reply->set_numreplay(size);
        return Status::OK;
    }

public:
    RPCThread * thread_;
};

void kvserver::RunServer(RPCThread * thread) {

    // thread_ = thread;
    std::string server_address("0.0.0.0:50051");
    //   GreeterServiceImpl service;
    PageLoaderServiceImpl service(thread);

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