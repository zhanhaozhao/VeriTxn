
#include "global_struct.h"
#include "helper.h"
#include "rpc_thread.h"
#include "wl.h"
#include "re_ycsb_txn.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "rocksdb/db.h"
#include "rocksdb/convenience.h"
#include "atomic"
#include "kvengine.h"

void RPCThread::setup() {}

RC RPCThread::run() {
  tsetup();

  // pthread_barrier_wait(&log_bar);
  if (g_log_recover) {
    // server = new kvserver();
		// server->RunServer(this);

    struct sockaddr_in servaddr, cliaddr;

    int listenfd, connfd;
    pid_t childpid;

    char buf[MAX_LINE];

    socklen_t clilen;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket error");
      exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("bind port %d:%d error\n", servaddr.sin_port, PORT);
        perror("bind port error");
      exit(1);
    }

    if(listen(listenfd, LISTENQ) < 0) {
      perror("listen error");
      exit(1);
    }

    std::cout<< "Listening at:" << LISTENQ << std::endl;

    for( ; ; ) {
      clilen = sizeof(cliaddr);
      if((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0 ) {
        perror("accept error");
        exit(1);
      }

      if((childpid = fork()) == 0) {
        close(listenfd);
        ssize_t n;
        char buff[MAX_LINE];

        while((n = read(connfd, buff, MAX_LINE-1)) > 0) {
          // std::cout<< "received from client:" << buff << std::endl;
          buff[n] = '\0';
          char * prefix = (char *) malloc(4);
          uint32_t offset = 0;
          UNPACK_SIZE(buff, prefix, 4, offset);

          if (strcmp(prefix, "LOGS")) {
            // read page
            std::string page_id(buff);
//            printf("scanning prefix %s\n", page_id.c_str());
            // read from rocksdb
            // std::map<std::string> items;
            std::vector<std::string> reply;
            std::atomic<uint64_t> record_cnt;
            record_cnt = 0;
            auto f_proc_entry = [this, &reply, &record_cnt](const rocksdb::Iterator * it) {
//              Item* item = reply->add_dataitem();
//              item->set_key(it->key().data());
//              item->set_value(it->value().data());
//              items.emplace_back(it->value().data());
//                reply.emplace_back(it->value().data());
                record_cnt.fetch_add(1);
            };
           eng->DBPrefixScan(page_id, f_proc_entry);
//            printf("scan op = %lu\n", record_cnt.load());

            char * response = (char *) malloc(sizeof(long));
            sprintf(response, "%d", n);
            std::string str(response);
            write(connfd, str.c_str(), str.size());

          } else {
            uint64_t tt = get_sys_clock();
            uint32_t batch_size = 0;
            UNPACK(buff, batch_size, offset);
            // std::cout<< "received from client:" << prefix << " number of logs:" << batch_size <<std::endl;
            for (int i = 0; i < batch_size; i++) {

              uint32_t entry_size = 0;
              UNPACK(buff, entry_size, offset);
              char * entry = (char *) malloc(entry_size);
              UNPACK_SIZE(buff, entry, entry_size, offset);

              re_txn_man * m_txn;
              uint64_t thd_id = this->get_thd_id();

              assert (glob_manager);
              switch (WORKLOAD) {
              case YCSB :
                  // m_txn = (ycsb_txn_man *) aligned_alloc(64, sizeof(ycsb_txn_man));
                  m_txn = (re_ycsb_txn_man *) malloc(sizeof(re_ycsb_txn_man));
                  new(m_txn) re_ycsb_txn_man();
                  break;
              case TPCC :
              // 	// m_txn = (tpcc_txn_man *) aligned_alloc(64, sizeof(tpcc_txn_man));
              	m_txn = (tpcc_txn_man *) malloc(sizeof(tpcc_txn_man));
              	new(m_txn) tpcc_txn_man();
              // 	break;
              default:
                  assert(false);
              }

              m_txn->init(this, this->_wl, thd_id);
              glob_manager->set_txn_man(m_txn);
              assert (m_txn);

              m_txn->set_txn_id(thd_id + glob_manager->get_thd_txn_id(thd_id) * g_thread_cnt);
              glob_manager->set_thd_txn_id(thd_id);

              //if (get_thd_id() == 0)
              COMPILER_BARRIER
              m_txn->recover_txn(entry);
              COMPILER_BARRIER
            }
            // std::cout << "time consumed:" << get_sys_clock() - tt << std::endl;

            // construct reposnse
//            char * response = (char *) malloc(sizeof(uint32_t));
//            sprintf(response, "%d", batch_size);
//            std::string str(response);
//            write(connfd, str.c_str(), str.size());
          }

        }
          printf("thread connection closed");
//          assert(false);
//        exit(0);    // why exit 0 ???
      }
      close(connfd);
    }
    close(listenfd);
  }

  return FINISH;
}


