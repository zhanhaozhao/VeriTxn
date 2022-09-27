#ifndef _GLOBAL_STRUCT_H_
#define _GLOBAL_STRUCT_H_

#include "global.h"

#include "mem_alloc.h"
#include "stats.h"
// #include "dl_detect.h"
// #include "manager.h"
#include "query.h"
// #include "helper.h"
#include "manager.h"

// #include "common/stats.h"
// #include "dl_detect.h"

// using namespace std;

// class Stats;
// class Manager;

/******************************************/
// Global Data Structure 
/******************************************/
extern mem_alloc mem_allocator;
extern Stats stats;
// extern DL_detect dl_detector;
extern Manager * glob_manager;
extern Query_queue * query_queue;
// extern Plock part_lock_man;
// extern OptCC occ_man;


#endif