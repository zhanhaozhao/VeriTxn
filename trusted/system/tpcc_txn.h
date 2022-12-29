#ifndef TPCC_TXN_H
#define TPCC_TXN_H


#include "txn.h"
#include "tpcc.h"

class tpcc_txn_man : public txn_man
{
public:
	void init(thread_t * h_thd, workload * h_wl, uint64_t part_id); 
	RC run_txn(base_query * query);
private:
	tpcc_wl * _wl;
	RC run_payment(tpcc_query * m_query);
	RC run_new_order(tpcc_query * m_query);
	RC run_order_status(tpcc_query * query);
	RC run_delivery(tpcc_query * query);
	RC run_stock_level(tpcc_query * query);

    row_t *stock_level_getOId(uint64_t d_w_id, uint64_t d_id);

    bool
    stock_level_getStockCount(uint64_t ol_w_id, uint64_t ol_d_id, int64_t ol_o_id, uint64_t s_w_id, uint64_t threshold,
                              uint64_t *out_distinct_count);
    RC get_new_row(std::string t_name, row_t *&row, uint64_t part_id, uint64_t &row_id);
};

#endif