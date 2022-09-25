#ifndef _THREAD_ENC_
#define _THREAD_ENC_
#include "txn.h"
#include "thread.h"

void global_init_ecall();

void init_txn_in_enc(txn_man *& m_txn, thread_t * h_thd);
RC run_txn_ecall(thread_t * h_thd, ts_t starttime);

RC runTest(txn_man * txn);

#endif
