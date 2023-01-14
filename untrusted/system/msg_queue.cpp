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

#include "msg_queue.h"
#include "mem_alloc.h"
#include "query.h"
// #include "pool.h"
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include "message.h"

void MessageQueue::init() {
  //m_queue = new boost::lockfree::queue<msg_entry* > (0);
  m_queue = new boost::lockfree::queue<msg_entry* > * [OUTPUT_CNT];
  for(uint64_t i = 0; i < OUTPUT_CNT; i++) {
    m_queue[i] = new boost::lockfree::queue<msg_entry* > (0);
  }
  ctr = new  uint64_t * [OUTPUT_CNT];
  for(uint64_t i = 0; i < OUTPUT_CNT; i++) {
    ctr[i] = (uint64_t*) malloc(sizeof(uint64_t));
    *ctr[i] = i % g_thread_cnt;
  }
  msg_queue_size=0;
  // sem_init(&_semaphore, 0, 1);
  for (uint64_t i = 0; i < OUTPUT_CNT; i++) sthd_m_cache.push_back(NULL);
}

uint64_t msg_count = 0;

void MessageQueue::enqueue(uint64_t thd_id, Message * msg,uint64_t dest) {
  assert(dest < NODE_CNT);
  assert(dest != g_node_id);


  msg_entry * entry = (msg_entry*) malloc(sizeof(struct msg_entry));
  //msg_pool.get(entry);
  entry->msg = msg;
  entry->dest = dest;
  entry->starttime = get_sys_clock();
  entry->touchedtime = UINT64_MAX;
  // assert(entry->dest < g_total_node_cnt);
  uint64_t mtx_time_start = get_sys_clock();
  uint64_t rand = mtx_time_start % OUTPUT_CNT;
  uint64_t cur_cnt = ATOM_ADD_FETCH(msg_count, 1);
  if (cur_cnt % 200000 == 0) {
//      printf("enqueue! msg to %lu cnt %lu\n", entry->dest, cur_cnt);
  }

  while (!m_queue[rand]->push(entry)/* && !simulation->is_done()*/) {
  }
}

uint64_t output_msg = 0;

uint64_t MessageQueue::dequeue(uint64_t thd_id, Message *& msg) {
  msg_entry * entry = NULL;
  uint64_t dest = UINT64_MAX;
  uint64_t mtx_time_start = get_sys_clock();
  bool valid = false;

  valid = m_queue[thd_id%OUTPUT_CNT]->pop(entry);

  uint64_t curr_time = get_sys_clock();
  if(valid) {
    assert(entry);
    uint64_t cur_cnt = ATOM_ADD_FETCH(output_msg, 1);
    if(entry->touchedtime == UINT64_MAX) entry->touchedtime = get_sys_clock(); //record first touch

    dest = entry->dest;
    // assert(dest < g_total_node_cnt);
    msg = entry->msg;
  } else {
    msg = NULL;
    dest = UINT64_MAX;
  }
  return dest;
}
