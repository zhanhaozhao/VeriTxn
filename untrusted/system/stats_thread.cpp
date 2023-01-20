//
// Created by pan on 2023/1/12.
//

#include "stats_thread.h"
#include <unistd.h>
#include "common/config.h"

void StatsThread::setup() {}

RC StatsThread::run() {
    tsetup();

    printf("Running stats thread\n");
    uint64_t point_cnt = 0;
    for (;!_wl->sim_done;) {
        usleep(1000 * 10);
        if (STATS_ENABLE) {
            stats.print();
            for (int i=0;i<THREAD_CNT;i++)
                stats.clear(i);
        }
        printf("currently is point %lu\n", point_cnt++);
    }

    return FINISH;
}
