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

#include "mem_alloc.h"
#include "query.h"
#include "global.h"
#include "message.h"

std::vector<Message*> * Message::create_messages(char * buf) {
    std::vector<Message*> * all_msgs = new std::vector<Message*>;
    char * data = buf;
    uint64_t ptr = 0;
    uint64_t starttime = 0;
    uint32_t dest_id;
    uint32_t return_id;
    uint32_t txn_cnt;
    COPY_VAL(dest_id,data,ptr);
    COPY_VAL(return_id,data,ptr);
    COPY_VAL(txn_cnt,data,ptr);
    COPY_VAL(starttime,data,ptr);

    assert(dest_id == g_node_id);
    assert(return_id != g_node_id);
    while(txn_cnt > 0) {
        Message * msg = create_message(&data[ptr]);
        msg->return_node_id = return_id;
        ptr += msg->get_size();
        all_msgs->push_back(msg);
        --txn_cnt;
    }
    return all_msgs;
}

Message * Message::create_message(char * buf) {
    RemReqType rtype = NO_MSG;
    uint64_t ptr = 0;
    COPY_VAL(rtype,buf,ptr);
    Message * msg = create_message(rtype);
    //printf("buffer is:%s\n",buf);
    //printf("msg:%lu:%lu %lu %lu\n",((DAQueryMessage*)msg)->seq_id,((DAQueryMessage*)msg)->state,((DAQueryMessage*)msg)->next_state,((DAQueryMessage*)msg)->last_state);
    fflush(stdout);
    msg->copy_from_buf(buf);
    return msg;
}

Message * Message::create_message(RemReqType rtype) {
    Message * msg;
    switch(rtype) {
        case ASYNC_HASH:
            msg = new AsyncHashMessage;
            break;
        case INIT_DONE:
            msg = new InitDoneMessage;
            break;
        default:
            assert(false);
    }
    assert(msg);
    msg->rtype = rtype;
    msg->txn_id = UINT64_MAX;
    msg->return_node_id = g_node_id;

    return msg;
}

uint64_t Message::mget_size() {
    uint64_t size = 0;
    size += sizeof(RemReqType);
    size += sizeof(uint64_t);
    return size;
}

void Message::mcopy_from_buf(char * buf) {
    uint64_t ptr = 0;
    COPY_VAL(txn_id,buf,ptr);
}

void Message::mcopy_to_buf(char * buf) {
    uint64_t ptr = 0;
    COPY_BUF(buf,rtype,ptr);
    COPY_BUF(buf,txn_id,ptr);
}

void Message::release_message(Message * msg) {
    switch(msg->rtype) {
        case ASYNC_HASH:{
            AsyncHashMessage * m_msg = (AsyncHashMessage*)msg;
            m_msg->release();
            delete m_msg;
            break;
        }
        case INIT_DONE: {
            InitDoneMessage * m_msg = (InitDoneMessage*)msg;
            m_msg->release();
            delete m_msg;
            break;
        }
        default: {
            assert(false);
        }
    }
}
/************************/

void AsyncHashMessage::init(std::string name, int pid, uint64_t bid, uint64_t h, uint64_t _ts) {
    // index_name = name;
    strcpy(index_name, name.c_str());
    part_id = pid;
    bkt_idx = bid;
    hash = h;
    ts = _ts;
}

uint64_t AsyncHashMessage::get_size() {
    uint64_t size = Message::mget_size();
    size+=sizeof(char) * INDEX_NAME_LENGTH;
    size+=sizeof(int);
    size+=sizeof(uint64_t);
    size+=sizeof(uint64_t);
    size+=sizeof(uint64_t);
    return size;
}

void AsyncHashMessage::copy_from_buf(char * buf) {
    Message::mcopy_from_buf(buf);
    uint64_t ptr = Message::mget_size();
    COPY_VAL(index_name,buf,ptr);
    COPY_VAL(part_id,buf,ptr);
    COPY_VAL(bkt_idx,buf,ptr);
    COPY_VAL(hash,buf,ptr);
    COPY_VAL(ts,buf,ptr);
    assert(ptr == get_size());
}

void AsyncHashMessage::copy_to_buf(char * buf) {
    Message::mcopy_to_buf(buf);
    uint64_t ptr = Message::mget_size();
    COPY_BUF(buf,index_name,ptr);
    COPY_BUF(buf,part_id,ptr);
    COPY_BUF(buf,bkt_idx,ptr);
    COPY_BUF(buf,hash,ptr);
    COPY_BUF(buf,ts,ptr);
    assert(ptr == get_size());
}

uint64_t InitDoneMessage::get_size() {
  uint64_t size = Message::mget_size();
  return size;
}

void InitDoneMessage::copy_from_buf(char* buf) { Message::mcopy_from_buf(buf); }

void InitDoneMessage::copy_to_buf(char* buf) { Message::mcopy_to_buf(buf); }