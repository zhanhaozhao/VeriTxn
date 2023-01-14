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

#include "global.h"
#include "helper.h"
#include "manager.h"
#include "thread.h"
#include "io_thread.h"
// #include "query.h"
#include "mem_alloc.h"
#include "transport.h"
#include "math.h"
#include "msg_queue.h"
#include "msg_thread.h"
#include "message.h"
// #include "txn.h"
#include "ycsb.h"
#include "sim_manager.h"
#include "api.h"


void InputThread::setup() {

	std::vector<Message*> * msgs;
	while(!simulation->is_setup_done()) {
		msgs = tport_man.recv_msg(get_thd_id());
		if (msgs == NULL) continue;
		while(!msgs->empty()) {
			Message * msg = msgs->front();
			if(msg->rtype == INIT_DONE) {
				printf("Received INIT_DONE from node %ld\n",msg->return_node_id);
				fflush(stdout);
				simulation->process_setup_msg();
			} else {
				// work_queue.enqueue(get_thd_id(),msg,false);
			}
			msgs->erase(msgs->begin());
		}
		delete msgs;
	}
}

RC InputThread::run() {
	tsetup();
	printf("Running InputThread %ld\n",_thd_id);
	server_recv_loop();

	return FINISH;
}

RC InputThread::server_recv_loop() {

	myrand rdm;
	rdm.init(get_thd_id());
	RC rc = RCOK;
	assert (rc == RCOK);
	uint64_t starttime;

	std::vector<Message*> * msgs;

	while (true) {
		if (USE_ASYNC_HASH != 1) return FINISH;
		heartbeat();
		starttime = get_sys_clock();

		msgs = tport_man.recv_msg(get_thd_id());

		// INC_STATS(_thd_id,mtx[28], get_sys_clock() - starttime);
		// starttime = get_sys_clock();

		if (msgs == NULL) continue;
		while(!msgs->empty()) {
			Message * msg = msgs->front();
			if(msg->rtype == INIT_DONE) {
				msgs->erase(msgs->begin());
				continue;
			}
			if (msg->rtype == ASYNC_HASH) {
				AsyncHashMessage* amsg = (AsyncHashMessage*)msg;
				std::string index_name = amsg->index_name;
				update_hash_value(index_name, amsg->part_id, amsg->bkt_idx, amsg->hash, amsg->ts);
			}
			// DEBUG("recv txn %ld type %d\n",msg->get_txn_id(),msg->get_rtype());
			// work_queue.enqueue(get_thd_id(),msg,false);
			msgs->erase(msgs->begin());
		}
		delete msgs;
		// INC_STATS(_thd_id,mtx[29], get_sys_clock() - starttime);
		if (_wl->sim_done) {
   		    return FINISH;
   		}
	}
	printf("FINISH %ld:%ld\n",_node_id,_thd_id);
	fflush(stdout);
	return FINISH;
}

void OutputThread::setup() {
	// DEBUG_M("OutputThread::setup MessageThread alloc\n");
	messager = (MessageThread *) malloc(sizeof(MessageThread));
	messager->init(_thd_id);
	while (!simulation->is_setup_done()) {
		messager->run();
	}
}


RC OutputThread::run() {

	tsetup();
	setup();
	printf("Running OutputThread %ld\n",_thd_id);
	while (true) {
		if (USE_ASYNC_HASH != 1) return FINISH;
		heartbeat();
		messager->run();
		if (_wl->sim_done) {
   		    return FINISH;
   		}
	}
    printf("Closing OutputThread %ld\n",_thd_id);
	//extra_wait_time should be as small as possible
	// printf("FINISH %ld:%ld, extra wait time: %lu(ns)\n",_node_id,_thd_id,extra_wait_time);
	fflush(stdout);
	return FINISH;
}


