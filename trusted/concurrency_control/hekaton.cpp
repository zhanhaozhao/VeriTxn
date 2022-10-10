#include "txn.h"
#include "row_enc.h"
#include "row_hekaton.h"
#include "manager_enc.h"

#if CC_ALG==HEKATON

RC
txn_man::validate_hekaton(RC rc)
{
//	uint64_t starttime = get_cur_time_ocall();
//	INC_STATS_ENC(get_thd_id(), debug1, get_cur_time_ocall() - starttime);
	ts_t commit_ts = glob_manager_enc->get_ts(get_thd_id());
	// validate the read set.
#if ISOLATION_LEVEL == SERIALIZABLE
	if (rc == RCOK) {
		for (int rid = 0; rid < row_cnt; rid ++) {
			if (accesses[rid]->type == WR)
				continue;
			rc = accesses[rid]->orig_row->manager->prepare_read(this, accesses[rid]->data, commit_ts);
			if (rc == Abort)
				break;
		}
	}
#endif
	// postprocess 
	for (int rid = 0; rid < row_cnt; rid ++) {
		if (accesses[rid]->type == RD)
			continue;
		accesses[rid]->orig_row->manager->post_process(this, commit_ts, rc);
	}
	return rc;
}

#endif
