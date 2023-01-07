#ifndef TPCC_H_
#define TPCC_H_

#include "wl.h"
// #include "txn.h"

// class table_t;
// class INDEX;
// class tpcc_query;



class tpcc_wl : public workload {
public:
	RC init();
	RC init_table();
	RC init_schema(const char * schema_file);
	// RC get_txn_man(txn_man *& txn_manager, thread_t * h_thd);
	table_t * 		t_warehouse;
	table_t * 		t_district;
	table_t * 		t_customer;
	table_t *		t_history;
	table_t *		t_neworder;
	table_t *		t_order;
	table_t *		t_orderline;
	table_t *		t_item;
	table_t *		t_stock;

	INDEX * 	i_item;
	INDEX * 	i_warehouse;
	INDEX * 	i_district;
	INDEX * 	i_customer_id;
	INDEX * 	i_customer_last;
	INDEX * 	i_stock;
    INDEX * 	i_order; // key = (w_id, d_id, o_id)
    INDEX * 	i_neworder;
	INDEX * 	i_orderline; // key = (w_id, d_id, o_id)
	// TODO: do not verify the cache for secondary indexes.
	
	bool * delivering;
	uint32_t next_tid;
private:
	uint64_t num_wh;
	void init_tab_item();
	void init_tab_wh(uint32_t wid);
	void init_tab_dist(uint64_t w_id);
	void init_tab_stock(uint64_t w_id);
	void init_tab_cust(uint64_t d_id, uint64_t w_id);
	void init_tab_hist(uint64_t c_id, uint64_t d_id, uint64_t w_id);
	void init_tab_order(uint64_t d_id, uint64_t w_id);
	
	void init_permutation(uint64_t * perm_c_id, uint64_t wid);

	static void * threadInitItem(void * This);
	static void * threadInitWh(void * This);
	static void * threadInitDist(void * This);
	static void * threadInitStock(void * This);
	static void * threadInitCust(void * This);
	static void * threadInitHist(void * This);
	static void * threadInitOrder(void * This);

	void * threadInitWarehouse(void * This);
};

struct thr_args{
	tpcc_wl * wl;
	UInt32 id;
	UInt32 tot;
};

#endif
