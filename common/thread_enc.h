#ifndef _THREAD_ENC_H_
#define _THREAD_ENC_H_

#include <string>
// #include "txn.h"
// #include "stats.h"
// #include "common/thread.h"

// void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd);
int runTest(void * txn);

void global_init_ecall(void * stats);
void index_init_ecall(int part_cnt, void * table, std::string iname, uint64_t bucket_cnt);
int run_txn_ecall(void * h_thd, uint64_t starttime);

#endif
