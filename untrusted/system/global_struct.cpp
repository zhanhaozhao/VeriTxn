#include "global_struct.h"

// #include "plock.h"
// #include "occ.h"

mem_alloc mem_allocator;
Stats stats;
// DL_detect dl_detector;

Manager * glob_manager;
Query_queue * query_queue;
table_map * global_table_map = new table_map;

Logger * logger;
RemoteStorage * remotestorage;

#if USE_SGX != 1
kvengine * eng;
#endif


// Plock part_lock_man;
// OptCC occ_man;