import os, sys, re, os.path
import platform
import string
import subprocess, datetime, time, signal

from collections import defaultdict, OrderedDict


def replace(filename, pattern, replacement):
    f = open(filename)
    s = f.read()
    f.close()
    s = re.sub(pattern, replacement, s)
    f = open(filename, 'w')
    f.write(s)
    f.close()


jobs = OrderedDict()
dbms_cfg = ["config-std.h", "common/config.h"]
algs = ['NO_WAIT']
count_job = 0


def insert_job(alg="OCC", workload="YCSB", thread_num=4, theta=0.6, bkt_fac=1, read_perc=0.5, use_sgx=True,
               cs=1024 * 1024 * 1024, veri="PAGE_VERI", index="IDX_HASH", pre_load=1, use_log=0, txn_per_thd=10000,
               database_size=1024 * 1024, txn_length=64, enable_data_cache=True, pt=1, prof="false", wh=4):
    global count_job
    count_job = count_job + 1
    jobs[count_job] = {
        "WORKLOAD"		: workload,
		"CORE_CNT"			: thread_num,
		"CC_ALG"			: alg,
		"THREAD_CNT"		: thread_num,
		"ZIPF_THETA"		: theta,
		"BUCKET_FACTOR"		: bkt_fac,
		"READ_PERC"			: read_perc,
		"WRITE_PERC"		: 1-read_perc,
		"USE_SGX"			: 1 if use_sgx else 0,
		"VERIFIED_CACHE_SIZ": cs,
		"ENABLE_DATA_CACHE" : "true" if enable_data_cache else "false",
		"VERI_TYPE"			: veri,
		"INDEX_STRUCT"		: index,
		"PRE_LOAD"			: pre_load,
		"USE_LOG"			: use_log,
		"MAX_TXN_PER_PART"	: txn_per_thd,
		"SYNTH_TABLE_SIZE"	: database_size,
		"REQ_PER_QUERY"		: txn_length,
        "PART_CNT"          : pt,
        "PROFILING"         : "false",
        "NUM_WH"            : wh
	}


def test_compile(job):
    os.system("make clean> temp.out 2>&1")
    os.system("cp "+ dbms_cfg[0] +' ' + dbms_cfg[1])
    if job["USE_SGX"] == 1:
        os.system("make sgx-release 2>&1")
    else:
        os.system("make no-sgx 2>&1")

    for (param, value) in job.items():
        pattern = r"\#define\s*" + re.escape(param) + r'.*'
        replacement = "#define " + param + ' ' + str(value)
        replace(dbms_cfg[1], pattern, replacement)

    # print("clean finished!!!!")
    os.system("rm -f storage/rocksdb/* 2>&1")
    time.sleep(0.5)
    # if job["USE_LOG"] == 1:
    #     ret = os.system("cd script && bash ./storage_compile.sh> temp.out 2>&1")
    #     if ret != 0:
    #         print("ERROR in compiling job=")
    #         print(job)
    #         return False
    #     ret = os.system("rm -f storage/rocksdb/* 2>&1")
    #     if ret != 0:
    #         print("ERROR in compiling job=")
    #         print(job)
    #         return False
    os.system("make clean> temp.out 2>&1")
    os.system("cat common/config.h | grep USE_SGX")
    ret = os.system("make -j10> temp.out")
    # print("make finished!!!!")
    if ret != 0:
        print("ERROR in compiling job=")
        print(job)
        return False
    print("PASS Compile")
    return True


def test_run(job, f, test=''):
    global process_store
    print(job)
    app_flags = ""  # m_txn->run_txn
    if test == 'read_write':
        app_flags = "-Ar -t1"
    if test == 'conflict':
        app_flags = "-Ac -t4"

    if job["USE_LOG"] == 1:
        process_store = subprocess.Popen("./Store>./store.log", stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                         shell=True)
        time.sleep(7)
    cmd = "./App %s" % (app_flags)  # + fimeName
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    result = process.communicate()
    res = result[0].decode("utf-8")
    print(res)
    # print("input", res[res[0].find('[summary]'):])
    f.write(res[res.find('[summary]'):])
    process.wait()
    f.flush()
    if job["USE_LOG"] == 1:
        process_store.kill()


def test():
    binx = (b'g_flush_blocksize=1048576, g_log_buffer_size=209715200\nInitializing trusted log generator... Log buffer size 209715200\nDone\nInitializing message queue... Done\nInitializing transport manager... Tport Init 0: 3\nReading ifconfig file: ./ifconfig.txt\n0: 127.0.0.1\n1 1\nDone\nremotestorage client in main thread initialized!\nInitializing simulation... Done\nbegin to init [YCSB] Table.\nquery_queue initialized!\nSetup 0:0\nSetup 0:1\nRunning 0:1\nRunning 0:0\n[summary] total_txn_cnt=10000, total_abort_cnt=0, run_time=0.720400, time_wait=0.000000, time_ts_alloc=0.000322, time_man=0.000000, time_index=0.000000, time_abort=0.000000, time_cleanup=0.000000, time_cache=0.000000, latency=0.000072, deadlock_cnt=0, cycle_detect=0, dl_detect_time=0.000000, dl_wait_time=0.000000, time_query=0.000487, debug1=0.000000, debug2=0.000000, debug3=0.000000, debug4=0.000000, debug5=0.000000\n', b'')
    res = binx[0].decode("utf-8")
    print(res)
    print(res[res.find('[summary]'):])

testRound = 1


def run_all_test(jobs, filename):
    filename = "./results/" + filename
    os.system("echo 'thread_cnt, txn_cnt, abort_cnt, execution_time, latency' > %s" % filename)
    f = open(filename, 'w+')
    for (_, job) in jobs.items():
        for ii in range(testRound):
            while True:
                if test_compile(job):
                    break
            test_run(job, f)
            os.system("make clean> temp.out 2>&1")
    f.close()


# run YCSB tests
def run_thread_exp():
    global jobs
    jobs = OrderedDict()
    # [1, 2, 3, 4, 5, 6, 7, 8]
    for th in [1, 4, 8]:
        for alg in algs:
            insert_job(alg, 'YCSB', thread_num=th, use_sgx=False, txn_length=1)
    # for th in [1, 2, 3, 4, 5, 6, 7, 8]:
    #     for alg in algs:
    #         insert_job(alg, 'YCSB', thread_num=th, use_sgx=True)
    run_all_test(jobs, "thread_ycsb.csv")


def run_tpc_exp():
    global jobs
    jobs = OrderedDict()
    for th in [1, 2, 3, 4, 5, 6, 7, 8]:
        for alg in algs:
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, pt=8)
    for th in [1, 2, 3, 4, 5, 6, 7, 8]:
        for alg in algs:
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=True, pt=8)
    run_all_test(jobs, "thread_tpc.log")


def run_theta_exp():
    global jobs
    jobs = OrderedDict()
    # for th in [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]:
    #     for alg in algs:
    #         insert_job(alg, 'YCSB', theta=th, use_sgx=False)
    for th in [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]:
        for alg in algs:
            insert_job(alg, 'YCSB', theta=th, use_sgx=True)
    run_all_test(jobs, "theta_ycsb.csv")

#
# def run_bktsiz_exp():
#     global jobs
#     jobs = OrderedDict()
#     for th in [1, 4, 16, 32, 64, 128, 256]:
#         for alg in algs:
#             insert_job(alg, 'YCSB', bkt_fac=th, use_sgx=False)
#     # print(jobs)
#     run_all_test(jobs, "bucket_siz.csv")
#     jobs = OrderedDict()
#     for th in [1, 4, 16, 32, 64, 128, 256]:
#         for alg in algs:
#             insert_job(alg, 'YCSB', bkt_fac=th, use_sgx=True)
#     # print(jobs)
#     run_all_test(jobs, "bucket_siz_sgx.csv")


def run_rw_exp():
    global jobs
    jobs = OrderedDict()
    for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
        for alg in algs:
            insert_job(alg, 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=False)
    for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
        for alg in algs:
            insert_job(alg, 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=True)
    # # print(jobs)
    run_all_test(jobs, "rw_ycsb.csv")


# jobs = OrderedDict()
# for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
# 	for alg in algs:
# 		insert_job(alg, 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=True)
# # print(jobs)
# run_all_test(jobs, "rw_ycsb_sgx.csv")

# # run TPCC tests
# jobs = {}
# for alg in algs:
# 	insert_job(alg, 'TPCC')
# run_all_test(jobs)

def run_common_test():
    global jobs
    jobs = OrderedDict()
    # for th in [3, 6, 7]:
    # insert_job("OCC", 'YCSB', use_sgx=False)
    insert_job("NO_WAIT", 'YCSB', use_sgx=True)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True)
    # print(jobs)
    run_all_test(jobs, "comparison.csv")


def run_cache_size_impact_for_different_methods_test():
    global jobs
    jobs = OrderedDict()
    x_con = [1 * 1024 * 1024, 64 * 1024 * 1024, 128 * 1024 * 1024, 256 * 1024 * 1024, 512 * 1024 * 1024,
             1024 * 1024 * 1024]
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, cs=cs)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", cs=cs, pre_load=0)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", cs=cs, pre_load=0,
                   txn_per_thd=1000)
    run_all_test(jobs, "cache_size.log")


# def run_database_cache_size_impact_under_different_skew_test():
# 	global jobs
# 	jobs = OrderedDict()
# 	# [1*1024*1024, 4*1024*1024, 16*1024*1024, 64*1024*1024, 256*1024*1024, 1024*1024*1024]
# 	x_con = [128*1024*1024, 512*1024*1024]
# 	for theta in [0, 0.9]:	# 8kb per bucket.
# 		for cs in x_con:
# 			insert_job('OCC', 'YCSB', use_sgx=True, theta=theta, cs=cs)
# 	run_all_test(jobs, "db_skew_cache.csv")

def run_database_skew_test():
    global jobs
    jobs = OrderedDict()
    x_con = [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]
    for cs in x_con:
    	insert_job('NO_WAIT', 'YCSB', use_sgx=True, theta=cs)
    for cs in x_con:
    	insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", theta=cs, pre_load=0)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", theta=cs, pre_load=0,
                   txn_per_thd=1000)
    run_all_test(jobs, "cache_skew.log")


def run_database_size_test():
    global jobs
    jobs = OrderedDict()
    # 1 * 1024 * 1024, 8 * 1024 * 1024, 16 * 1024 * 1024, 32 * 1024 * 1024,
    x_con = [1 * 1024 * 1024, 8 * 1024 * 1024, 16 * 1024 * 1024, 32 * 1024 * 1024, 64 * 1024 * 1024]
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, database_size=cs, txn_per_thd=1000)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", database_size=cs, pre_load=0, txn_per_thd=1000)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", database_size=cs, pre_load=0,
                   txn_per_thd=1000)
    run_all_test(jobs, "db_size.log")


def run_database_varying_txn_length():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 4, 16, 32, 64]
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=False, txn_length=cs)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, txn_length=cs)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", txn_length=cs, pre_load=0)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", txn_length=cs, pre_load=0,
                   txn_per_thd=1000)
    run_all_test(jobs, "txn_length.log")


def run_database_varying_txn_length_no_sgx():
    global jobs
    jobs = OrderedDict()
    # 1, 4, 16, 32, 64
    x_con = [128]
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=False, use_log=1, txn_length=cs, txn_per_thd=1000)
    for cs in x_con:
        insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, txn_length=cs, txn_per_thd=1000)
    run_all_test(jobs, "no_sgx_txn_length.log")

def run_single_layer_cache_exp():
    global jobs
    jobs = OrderedDict()
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1)
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False)
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, database_size=1024 * 8)  # small db.
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, database_size=1024 * 8)
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, theta=0.9)
    # insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, theta=0.9)
    insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, database_size=1024 * 1024 * 16)
    insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, database_size=1024 * 1024 * 16)
    insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, theta=0)
    insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, theta=0)
    run_all_test(jobs, "single_layer.log")

def run_profiling():
    global jobs
    jobs = OrderedDict()
    for th in [1]:
        for alg in ["NO_WAIT"]:
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, prof="true", txn_per_thd=1000)
    run_all_test(jobs, "profiling.log")
    # for th in [1, 2, 3, 4, 5, 6, 7, 8]:
    #     for alg in ["NO_WAIT"]:
    #         insert_job(alg, 'TPCC', thread_num=th, use_sgx=True, prof="true", txn_per_thd=1000)
    # for th in [1, 2, 3, 4, 5, 6, 7, 8]:
    #     for alg in ["NO_WAIT"]:
    #         insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, prof="true", txn_per_thd=1000)
    # run_all_test(jobs, "profiling.log")


# run_cache_size_impact_for_different_methods_test()
# run_database_skew_test()
# run_database_size_test()
# run_database_varying_txn_length()
# run_single_layer_cache_exp()
# run_database_varying_txn_length_no_sgx()
# run_database_skew_test()
# run_rw_exp()
# run_thread_exp()
run_tpc_exp()
# run_common_test()
# run_profiling()
# run_theta_exp()
# run_bktsiz_exp()
# test()