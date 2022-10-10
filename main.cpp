#include "common/thread_enc.h"
#include "untrusted/system/global.h"
#include "untrusted/system/global_struct.h"
#include "common/ycsb.h"
#include "common/tpcc.h"
#include "common/test.h"
#include "common/db_thread.h"
#include "untrusted/system/transport.h"
#include "untrusted/system/sim_manager.h"
#include "untrusted/system/io_thread.h"
#include "untrusted/system/msg_queue.h"
#include "untrusted/system/log_thread.h"
#include "untrusted/system/logger.h"
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
void * run_thread(void * id);
thread_t * m_thds;
InputThread * input_thds;
OutputThread * output_thds;
LogThread * log_thds;
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
  } else printf("Enclave launched with id: %ld.\n", enclave_id); 
#endif // USE_SGX

	parser(argc, argv);
	
	mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt); 
	stats.init();

	glob_manager = (Manager *) _mm_malloc(sizeof(Manager), 64);
	glob_manager->init();

	global_init_ecall(&stats); // call enclave
	// if (g_cc_alg == DL_DETECT) 
	// 	dl_detector.init();
	printf("mem_allocator initialized!\n");

	printf("Initializing trusted log generator... ");
	fflush(stdout);
	logger.init("logfile.log");
	printf("Done\n");

	// if (NODE_CNT > 1) {
	printf("Initializing message queue... ");
	fflush(stdout);
	msg_queue.init();
	printf("Done\n");

	printf("Initializing transport manager... ");
	fflush(stdout);
	tport_man.init();
	printf("Done\n");

	printf("Initializing simulation... ");
	fflush(stdout);
	simulation = new SimManager;
	simulation->init();
	printf("Done\n");
	// }
	
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
	printf("workload initialized!\n");

	uint64_t thd_cnt = g_thread_cnt;
	uint64_t rthd_cnt = NODE_CNT > 1 ? INPUT_CNT : 0;
	uint64_t sthd_cnt = NODE_CNT > 1 ? OUTPUT_CNT : 0;
	uint64_t log_cnt = 1;
	uint64_t all_thd_cnt = thd_cnt + rthd_cnt + sthd_cnt + log_cnt;
	input_thds = new InputThread[rthd_cnt];
	output_thds = new OutputThread[sthd_cnt];
	log_thds = new LogThread[1];

	// pthread_t p_thds[all_thd_cnt - 1];
	pthread_t p_thds[all_thd_cnt];
	// m_thds = new thread_t * [thd_cnt];
	m_thds = new thread_t[thd_cnt];
	// for (uint32_t i = 0; i < thd_cnt; i++)
	// 	m_thds[i] = (thread_t *) _mm_malloc(sizeof(thread_t), 64);
	// query_queue should be the last one to be initialized!!!
	// because it collects txn latency
	query_queue = (Query_queue *) _mm_malloc(sizeof(Query_queue), 64);
	if (WORKLOAD != TEST)
		query_queue->init(m_wl);
	pthread_barrier_init( &warmup_bar, NULL, all_thd_cnt );
	printf("query_queue initialized!\n");
	warmup_finish = true;
	// spawn and run txns again.
	int64_t starttime = get_server_clock();
	int id = 0;
	for (uint32_t i = 0; i < thd_cnt; i++) {
		uint64_t vid = i;
		m_thds[vid].init(i, g_node_id, m_wl);
		pthread_create(&p_thds[id++], NULL, run_thread, (void *)&m_thds[vid]);
	}
	for (uint64_t j = 0; j < rthd_cnt ; j++) {
		// assert(id >= thd_cnt && id < wthd_cnt + rthd_cnt);
		input_thds[j].init(id,g_node_id,m_wl);
		pthread_create(&p_thds[id++], NULL, run_thread, (void *)&input_thds[j]);
	}
	for (uint64_t j = 0; j < sthd_cnt; j++) {
		// assert(id >= wthd_cnt + rthd_cnt && id < wthd_cnt + rthd_cnt + sthd_cnt);
		output_thds[j].init(id,g_node_id,m_wl);
		pthread_create(&p_thds[id++], NULL, run_thread, (void *)&output_thds[j]);
	}
	log_thds[0].init(id,g_node_id,m_wl);
	pthread_create(&p_thds[id++], NULL, run_thread, (void *)&log_thds[0]);
	

	// m_thds[thd_cnt - 1]->init(i, g_node_id, m_wl);
	// run_thread((void *)(m_thds[thd_cnt - 1]));
	for (uint32_t i = 0; i < thd_cnt; i++) 
		pthread_join(p_thds[i], NULL);
	int64_t endtime = get_server_clock();
	
	if (WORKLOAD != TEST) {
		printf("PASS! SimTime = %ld\n", endtime - starttime);
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
	m_thds[tid].run();
	return NULL;
}

void * run_thread(void * id) {
	Thread * thd = (Thread *) id;
	thd->run();
	return NULL;
}