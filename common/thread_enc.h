#ifndef _THREAD_ENC_H_
#define _THREAD_ENC_H_

#include "txn.h"
#include "stats.h"
// #include "common/thread.h"

void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd);
RC runTest(txn_man * txn);

void global_init_ecall(Stats * stats);
void index_init_ecall(int part_cnt, table_t * table, std::string iname, uint64_t bucket_cnt);
RC run_txn_ecall(thread_t * h_thd, ts_t starttime);

#endif
