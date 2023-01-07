
#ifndef _RPCTHREAD_H_
#define _RPCTHREAD_H_

// #include "global.h"
#include "thread.h"

// class Workload;

class RPCThread : public Thread {
public:
	RC 			run();
  void    setup();
};

#endif
