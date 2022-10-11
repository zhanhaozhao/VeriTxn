/*
   Copyright 2016 Massachusetts Institute of Technology

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "transport.h"
#include <sys/socket.h>
#include <stdio.h>
#include "common/config.h"
#include <string>
#include "pthread.h"
#include <iostream>
#include <fstream>

#include "global.h"
#include "manager.h"
#include "message.h"
#include "nn.hpp"
#include "query.h"
#include "tpcc_query.h"

#define MAX_IFADDR_LEN 20 // max # of characters in name of address

void Transport::read_ifconfig(const char * ifaddr_file) {

    ifaddr = new char *[NODE_CNT];

    uint64_t cnt = 0;
	printf("Reading ifconfig file: %s\n",ifaddr_file);
    std::ifstream fin(ifaddr_file);
    std::string line;
	while (std::getline(fin, line)) {
        ifaddr[cnt] = new char[line.length()+1];
		strcpy(ifaddr[cnt],&line[0]);
        printf("%ld: %s\n",cnt,ifaddr[cnt]);
        cnt++;
    }
	printf("%lu %u\n", cnt, NODE_CNT);
	assert(cnt == NODE_CNT);
}

uint64_t Transport::get_socket_count() {
	uint64_t sock_cnt = 0;
    sock_cnt = (NODE_CNT)*2 + OUTPUT_CNT;
	return sock_cnt;
}

std::string Transport::get_path() {
	std::string path;

    char * cpath;
	cpath = getenv("SCHEMA_PATH");
    if(cpath == NULL)
        path = "./";
    else
        path = std::string(cpath);
    path += "ifconfig.txt";
	return path;

}
#if USE_NANOMSG==1
Socket * Transport::get_socket() {
    Socket * socket = (Socket*) malloc(sizeof(Socket));
    new(socket) Socket();
    int timeo = 1000; // timeout in ms
    int stimeo = 1000; // timeout in ms
    int opt = 0;
    socket->sock.setsockopt(NN_SOL_SOCKET,NN_RCVTIMEO,&timeo,sizeof(timeo));
    socket->sock.setsockopt(NN_SOL_SOCKET,NN_SNDTIMEO,&stimeo,sizeof(stimeo));
    // NN_TCP_NODELAY doesn't cause TCP_NODELAY to be set -- nanomsg issue #118
    socket->sock.setsockopt(NN_SOL_SOCKET,NN_TCP_NODELAY,&opt,sizeof(opt));
    return socket;
}
#else
int Transport::get_socket() {
    struct sockaddr_in socket = new struct sockaddr_in;
    int timeo = 1000; // timeout in ms
    int stimeo = 1000; // timeout in ms
    int opt = 0;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        std::cout<<"create listen socket error"<<std::endl;
        return -1;
    }
    socket->sin_family = AF_INET;
    socket->sin_addr.s_addr = htonl(INADDR_ANY);
    socket->sin_port = htons(13000);
    if(bind(listenfd, (struct sockaddr*) socket, sizeof(*socket))){
        std::cout<<"bind listen socket error"<<std::endl;
        return -1;
    }
    //启动监听
    if(listen(listenfd, SOMAXCONN) == -1){
        std::cout<<"listen error"<<std::endl;
        return -1;
    }
    return listenfd;

}
#endif

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id) {
  uint64_t port_id = TPORT_PORT;
  port_id += NODE_CNT * dest_node_id;
  port_id += src_node_id;
  return port_id;
}

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id,
								uint64_t send_thread_id) {
  uint64_t port_id = 0;
  port_id += NODE_CNT * dest_node_id;
  port_id += src_node_id;
  port_id += send_thread_id * NODE_CNT * NODE_CNT;
  port_id += TPORT_PORT;
  return port_id;
}

Socket * Transport::bind(uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
// #if TPORT_TYPE == IPC
//   sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
// #else
// /*#if ENVIRONMENT_EC2
//   sprintf(socket_name,"tcp://eth0:%ld",port_id);
// #else*/
  sprintf(socket_name,"tcp://%s:%ld",ifaddr[g_node_id],port_id);
//#endif
// #endif
  printf("Sock Binding to %s %d\n",socket_name,g_node_id);
  int rc = socket->sock.bind(socket_name);
  if(rc < 0) {
	printf("Bind Error: %d %s\n",errno,strerror(errno));
	assert(false);
  }
  return socket;
}

Socket * Transport::connect(uint64_t dest_id,uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
// #if TPORT_TYPE == IPC
//   sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
// #else
// /*#if ENVIRONMENT_EC2
//   sprintf(socket_name,"tcp://eth0;%s:%ld",ifaddr[dest_id],port_id);
// #else*/
  sprintf(socket_name,"tcp://%s;%s:%ld",ifaddr[g_node_id],ifaddr[dest_id],port_id);
//#endif
// #endif
  printf("Sock Connecting to %s %d -> %ld\n",socket_name,g_node_id,dest_id);


  int rc = socket->sock.connect(socket_name);
  if(rc < 0) {
	printf("Connect Error: %d %s\n",errno,strerror(errno));
	assert(false);
  }
  return socket;
}

void Transport::init() {
	_sock_cnt = get_socket_count();

	rr = 0;
	printf("Tport Init %d: %ld\n",g_node_id,_sock_cnt);

 	std::string path = get_path();
	read_ifconfig(path.c_str());

    for(uint64_t node_id = 0; node_id < NODE_CNT; node_id++) {
        if (node_id == g_node_id) continue;

        // Listening ports
        for (uint64_t server_thread_id = g_thread_cnt + INPUT_CNT;
            server_thread_id < g_thread_cnt + INPUT_CNT + OUTPUT_CNT;
            server_thread_id++) {
            uint64_t port_id = get_port_id(node_id,g_node_id,server_thread_id % OUTPUT_CNT);
            Socket * sock = bind(port_id);
            recv_sockets.push_back(sock);
        }
        // Sending ports
        for (uint64_t server_thread_id = g_thread_cnt + INPUT_CNT;
            server_thread_id < g_thread_cnt + INPUT_CNT + OUTPUT_CNT;
            server_thread_id++) {
            uint64_t port_id = get_port_id(g_node_id,node_id,server_thread_id % OUTPUT_CNT);
            std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,server_thread_id);
            Socket * sock = connect(node_id,port_id);
            send_sockets.insert(std::make_pair(sender,sock));
        }
        
    }
	  fflush(stdout);
}

// rename sid to send thread id
void Transport::send_msg(uint64_t send_thread_id, uint64_t dest_node_id, void * sbuf,int size) {
	uint64_t starttime = get_sys_clock();

	Socket * socket = send_sockets.find(std::make_pair(dest_node_id,send_thread_id))->second;

	void * buf = nn_allocmsg(size,0);
	memcpy(buf,sbuf,size);


  	int rc = -1;
    while (rc < 0 /*&& (!simulation->is_setup_done() ||
              (simulation->is_setup_done() && !simulation->is_done()))*/) {

      rc= socket->sock.send(&buf,NN_MSG,NN_DONTWAIT);
    }

}

// Listens to sockets for messages from other nodes
std::vector<Message*> * Transport::recv_msg(uint64_t thd_id) {
	int bytes = 0;
	void * buf;
  uint64_t starttime = get_sys_clock();
  std::vector<Message*> * msgs = NULL;
  //uint64_t ctr = starttime % recv_sockets.size();
  uint64_t rand = (starttime % recv_sockets.size()) / INPUT_CNT;
  // uint64_t ctr = ((thd_id % INPUT_CNT) % recv_sockets.size()) + rand *
  // INPUT_CNT;
  uint64_t ctr = thd_id % INPUT_CNT;
  if (ctr >= recv_sockets.size()) return msgs;
  if(INPUT_CNT < NODE_CNT) {
	ctr += rand * INPUT_CNT;
	while(ctr >= recv_sockets.size()) {
	  ctr -= INPUT_CNT;
	}
  }
  assert(ctr < recv_sockets.size());
  uint64_t start_ctr = ctr;

  while (bytes <= 0 /*&& (!simulation->is_setup_done() ||
						(simulation->is_setup_done() && !simulation->is_done()))*/) {
	Socket * socket = recv_sockets[ctr];
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);
	//printf("[recv buf] %s", buf);
	//sleep(1);
	//if (buf != NULL) {
	//  exit(1);
	//}
	//++ctr;
	ctr = (ctr + INPUT_CNT);

	if (ctr >= recv_sockets.size()) ctr = (thd_id % INPUT_CNT) % recv_sockets.size();
	if (ctr == start_ctr) break;
		if(bytes <= 0 && errno != 11) {
		  printf("Recv Error %d %s\n",errno,strerror(errno));
			nn::freemsg(buf);
		}

	if (bytes > 0) break;
	}

	if(bytes <= 0 ) {
    // INC_STATS(thd_id,msg_recv_idle_time, get_sys_clock() - starttime);
    return msgs;
	}

  // INC_STATS(thd_id,msg_recv_time, get_sys_clock() - starttime);
	// INC_STATS(thd_id,msg_recv_cnt,1);

	starttime = get_sys_clock();

  msgs = Message::create_messages((char*)buf);

	nn::freemsg(buf);


	// INC_STATS(thd_id,msg_unpack_time,get_sys_clock()-starttime);
  return msgs;
}
