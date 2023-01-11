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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "global.h"
#include "helper.h"
// #include "logger.h"
// #include "array.h"

// class ycsb_request;
// class LogRecord;
// struct Item_no;

class Message {
public:
  virtual ~Message(){}
  static Message * create_message(char * buf);
  static Message * create_message(RemReqType rtype);
  static std::vector<Message*> * create_messages(char * buf);
  static void release_message(Message * msg);
  RemReqType rtype;
  uint64_t txn_id;
  uint64_t return_node_id;

  // Collect other stats
  uint64_t mget_size();
  uint64_t get_txn_id() {return txn_id;}

  void mcopy_from_buf(char * buf);
  void mcopy_to_buf(char * buf);
  RemReqType get_rtype() {return rtype;}

  virtual uint64_t get_size() = 0;
  virtual void copy_from_buf(char * buf) = 0;
  virtual void copy_to_buf(char * buf) = 0;
  virtual void init() = 0;
  virtual void release() = 0;
};

// Message types
class AsyncHashMessage : public Message {
public:
  void copy_from_buf(char * buf);
  void copy_to_buf(char * buf);
  uint64_t get_size();
  void init(){}
  void init(std::string name, int pid, uint64_t bid, uint64_t h, uint64_t _ts);
  void release() {}

  char index_name[INDEX_NAME_LENGTH];
  int part_id;
  uint64_t bkt_idx;
  uint64_t hash;
  uint64_t ts;
};

class InitDoneMessage : public Message {
public:
  void copy_from_buf(char * buf);
  void copy_to_buf(char * buf);
  uint64_t get_size();
  void init() {}
  void release() {}
};

#endif