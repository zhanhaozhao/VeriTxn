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

#include "msg_thread.h"
#include "msg_queue.h"
#include "message.h"
#include "mem_alloc.h"
#include "transport.h"
#include "query.h"
#include "global.h"

void MessageThread::init(uint64_t thd_id) {
  buffer_cnt = NODE_CNT;
  buffer = (mbuf **) malloc(sizeof(mbuf*) * buffer_cnt);
  for(uint64_t n = 0; n < buffer_cnt; n++) {
    // DEBUG_M("MessageThread::init mbuf alloc\n");
    buffer[n] = (mbuf *)malloc(sizeof(mbuf));
    buffer[n]->init(n);
    buffer[n]->reset(n);
  }
  _thd_id = thd_id;
}

void MessageThread::check_and_send_batches() {
  uint64_t starttime = get_sys_clock();
  for(uint64_t dest_node_id = 0; dest_node_id < buffer_cnt; dest_node_id++) {
#if SEND_TO_SELF_PAHSE == 0
    if (dest_node_id == g_node_id) continue;
#endif
    if(buffer[dest_node_id]->ready()) {
      send_batch(dest_node_id);
    }
  }
  // INC_STATS(_thd_id,mtx[11],get_sys_clock() - starttime);
}

void MessageThread::send_batch(uint64_t dest_node_id) {
  uint64_t starttime = get_sys_clock();
    mbuf * sbuf = buffer[dest_node_id];
    assert(sbuf->cnt > 0);
	  ((uint32_t*)sbuf->buffer)[2] = sbuf->cnt;
    sbuf->set_send_time(get_sys_clock());
    tport_man.send_msg(_thd_id,dest_node_id,sbuf->buffer,sbuf->ptr);
    sbuf->reset(dest_node_id);
}

static uint64_t mget_size() {
    uint64_t size = 0;
    size += sizeof(RemReqType);
    size += sizeof(uint64_t);
    assert(false);
}

void MessageThread::run() {

  uint64_t starttime = get_sys_clock();
  Message * msg = NULL;
  uint64_t dest_node_id;
  mbuf * sbuf;

  dest_node_id = msg_queue.dequeue(get_thd_id(), msg);
  if(!msg) {
    // INC_STATS(_thd_id,mtx[9],get_sys_clock() - starttime);
    check_and_send_batches();
    return;
  }
  assert(msg);
  assert(dest_node_id < NODE_CNT);
  assert(dest_node_id != g_node_id);
  sbuf = buffer[dest_node_id];

  if(!sbuf->fits(msg->get_size())) {
    assert(sbuf->cnt > 0);
    send_batch(dest_node_id);
  }

  #if WORKLOAD == DA
  if(!is_server&&true)
  printf("cl seq_id:%lu type:%c trans_id:%lu item:%c state:%lu next_state:%lu write_version:%lu\n",
      ((DAClientQueryMessage*)msg)->seq_id,
      type2char1(((DAClientQueryMessage*)msg)->txn_type),
      ((DAClientQueryMessage*)msg)->trans_id,
      static_cast<char>('x'+((DAClientQueryMessage*)msg)->item_id),
      ((DAClientQueryMessage*)msg)->state,
      (((DAClientQueryMessage*)msg)->next_state),
      ((DAClientQueryMessage*)msg)->write_version);
      fflush(stdout);
  #endif

  uint64_t copy_starttime = get_sys_clock();
  msg->copy_to_buf(&(sbuf->buffer[sbuf->ptr]));
  // INC_STATS(_thd_id,msg_copy_output_time,get_sys_clock() - copy_starttime);
  // DEBUG("%ld Buffered Msg %d, (%ld,%ld) to %ld\n", _thd_id, msg->rtype, msg->txn_id, msg->batch_id,
  //       dest_node_id);
  sbuf->cnt += 1;
  sbuf->ptr += msg->get_size();
  // Free message here, no longer needed unless CALVIN sequencer

  Message::release_message(msg);
  
  if (sbuf->starttime == 0) sbuf->starttime = get_sys_clock();

  check_and_send_batches();
  // INC_STATS(_thd_id,mtx[10],get_sys_clock() - starttime);

}

