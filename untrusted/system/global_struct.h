#ifndef GLOBAL_STRUCT_H_
#define GLOBAL_STRUCT_H_

#include "global.h"

#include "mem_alloc.h"
#include "stats.h"
// #include "dl_detect.h"
// #include "manager.h"
#include "query.h"
// #include "helper.h"
#include "manager.h"
#include "common/table_map.h"
#include "logger.h"
#include "disk.h"
#include "kvengine.h"

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
extern table_map * global_table_map;
extern Logger * logger;
extern RemoteStorage *remotestorage;
#if USE_SGX != 1
extern kvengine * eng;
#endif
// extern Plock part_lock_man;
// extern OptCC occ_man;


#endif