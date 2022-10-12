#include "common/thread_enc.h"
#include "untrusted/system/global.h"
#include "untrusted/system/global_struct.h"
#include "common/ycsb.h"
#include "common/tpcc.h"
#include "common/test.h"
#include "common/db_thread.h"
// #include "manager.h"
// #include "common/mem_alloc.h"
// #include "common/query.h"
// #include "plock.h"
// #include "occ.h"

// #include "thread_enc.h"
// #include "api.h"

// #include "global_enc.h"

#include "common/config.h"
#ifdef USE_SGX
#include "sgx_urts.h"
#define ENCLAVE_FILENAME "Enclave.signed.so"
extern sgx_enclave_id_t enclave_id;
#endif // USE_SGX


void * f(void *);

thread_t ** m_thds;

// defined in parser.cpp
void parser(int argc, char * argv[]);


int main(int argc, char* argv[])
{

#ifdef USE_SGX
  sgx_launch_token_t t;
  int updated = 0;
  memset(t, 0, sizeof(sgx_launch_token_t));
  sgx_status_t enclave_status = sgx_create_enclave(ENCLAVE_FILENAME,
    SGX_DEBUG_FLAG, &t, &updated, &enclave_id, NULL);
  if (enclave_status != SGX_SUCCESS) {
    printf("Failed to create Enclave : error %d - %#x.\n", enclave_status,
      enclave_status);
    return 1;
  } //else printf("Enclave launched with id: %ld.\n", enclave_id);
#endif // USE_SGX

	parser(argc, argv);
	
	mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt); 
	stats.init();

	glob_manager = (Manager *) _mm_malloc(sizeof(Manager), 64);
	glob_manager->init();

	global_init_ecall(&stats); // call enclave
	// if (g_cc_alg == DL_DETECT) 
	// 	dl_detector.init();
//	printf("mem_allocator initialized!\n");

	workload * m_wl;
	switch (WORKLOAD) {
		case YCSB :
			m_wl = new ycsb_wl; break;
		case TPCC :
			m_wl = new tpcc_wl; break;
		case TEST :
			m_wl = new TestWorkload; 
			((TestWorkload *)m_wl)->tick();
			break;
		default:
			assert(false);
	}
	m_wl->init();
//	printf("workload initialized!\n");

	uint64_t thd_cnt = g_thread_cnt;
	pthread_t p_thds[thd_cnt - 1];
	m_thds = new thread_t * [thd_cnt];
	for (uint32_t i = 0; i < thd_cnt; i++)
		m_thds[i] = (thread_t *) _mm_malloc(sizeof(thread_t), 64);
	// query_queue should be the last one to be initialized!!!
	// because it collects txn latency
	query_queue = (Query_queue *) _mm_malloc(sizeof(Query_queue), 64);
	if (WORKLOAD != TEST)
		query_queue->init(m_wl);
	pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );
//	printf("query_queue initialized!\n");

// #if CC_ALG == HSTORE
// 	part_lock_man.init();
// #elif CC_ALG == OCC
// 	occ_man.init();

	for (uint32_t i = 0; i < thd_cnt; i++) 
		m_thds[i]->init(i, m_wl);

	if (WARMUP > 0){
		printf("WARMUP start!\n");
		for (uint32_t i = 0; i < thd_cnt - 1; i++) {
			uint64_t vid = i;
			pthread_create(&p_thds[i], NULL, f, (void *)vid);
		}
		f((void *)(thd_cnt - 1));
		for (uint32_t i = 0; i < thd_cnt - 1; i++)
			pthread_join(p_thds[i], NULL);
		printf("WARMUP finished!\n");
	}
	warmup_finish = true;
	pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );
// #ifndef NOGRAPHITE
// 	CarbonBarrierInit(&enable_barrier, g_thread_cnt);
// #endif
	pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );

	// spawn and run txns again.
//	int64_t starttime = get_server_clock();
	for (uint32_t i = 0; i < thd_cnt - 1; i++) {
		uint64_t vid = i;
		pthread_create(&p_thds[i], NULL, f, (void *)vid);
	}
	f((void *)(thd_cnt - 1));
	for (uint32_t i = 0; i < thd_cnt - 1; i++) 
		pthread_join(p_thds[i], NULL);
//	int64_t endtime = get_server_clock();
	
	if (WORKLOAD != TEST) {
//		printf("PASS! SimTime = %ld\n", endtime - starttime);
		if (STATS_ENABLE)
			stats.print();
	} else {
		((TestWorkload *)m_wl)->summarize();
	}

#ifdef USE_SGX
  enclave_status = sgx_destroy_enclave(enclave_id);
  assert(enclave_status == SGX_SUCCESS);
#endif // USE_SGX

	return 0;
}

void * f(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid]->run();
	return NULL;
}
