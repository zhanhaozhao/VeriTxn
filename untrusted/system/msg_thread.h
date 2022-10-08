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

#ifndef _MSG_THREAD_H_
#define _MSG_THREAD_H_

#include "global.h"
#include "helper.h"


#include "nn.hpp"


struct mbuf {
  char * buffer;
  uint64_t starttime;
  uint64_t ptr;
  uint64_t cnt;
  bool wait;

  void init(uint64_t dest_id) { buffer = (char *)nn_allocmsg(MSG_SIZE_MAX, 0); }
  void reset(uint64_t dest_id) {
    //buffer = (char*)nn_allocmsg(MSG_SIZE_MAX,0);
    //memset(buffer,0,MSG_SIZE_MAX);
    starttime = 0;
    cnt = 0;
    wait = false;
	  ((uint32_t*)buffer)[0] = dest_id;
	  ((uint32_t*)buffer)[1] = g_node_id;
    ptr = sizeof(uint32_t) * 3 + sizeof(uint64_t);
  }
  void set_send_time(uint64_t ts) {
    //buffer = (char*)nn_allocmsg(MSG_SIZE_MAX,0);
    //memset(buffer,0,MSG_SIZE_MAX);
    // starttime = 0;
    // cnt = 0;
    // wait = false;
    char * ts_ptr = buffer + sizeof(uint32_t) * 3;
	  ((uint64_t*)ts_ptr)[0] = ts;
  }
  void copy(char * p, uint64_t s) {
    assert(ptr + s <= MSG_SIZE_MAX);
    if (cnt == 0) starttime = get_sys_clock();
    COPY_BUF_SIZE(buffer,p,ptr,s);
    //memcpy(&((char*)buffer)[size],p,s);
    //size += s;
  }
  bool fits(uint64_t s) { return (ptr + s) <= MSG_SIZE_MAX; }
  bool ready() {
    if (cnt == 0) return false;
    if ((get_sys_clock() - starttime) >= MSG_TIME_LIMIT) return true;
    return false;
  }
};

class MessageThread {
public:
  void init(uint64_t thd_id);
  void run();
  void check_and_send_batches();
  void send_batch(uint64_t dest_node_id);
  uint64_t get_thd_id() { return _thd_id;}
private:
  mbuf ** buffer;
  uint64_t buffer_cnt;
  uint64_t _thd_id;

};

#endif
