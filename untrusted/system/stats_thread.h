//
// Created by pan on 2023/1/12.
//

#ifndef DBX1000_STATS_THREAD_H
#define DBX1000_STATS_THREAD_H

#include "global_struct.h"

class StatsThread : public Thread {
public:
    RC 			run();
    void setup();
};

#endif //DBX1000_STATS_THREAD_H
