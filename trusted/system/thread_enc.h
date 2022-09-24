#ifndef _THREAD_ENC_
#define _THREAD_ENC_
// #include "txn.h"
#include "thread.h"

void global_init();
void global_init2();

void run_txn_in_enc(thread_t * h_thd);

#endif
