import os, sys, re, os.path
import platform
import string
import subprocess, datetime, time, signal

from collections import defaultdict, OrderedDict

BigTest = True
KB = 1024
MB = 1024 * KB
GB = 1024 * MB
ReadOnly = False

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


def insert_job(alg="OCC", workload="YCSB", thread_num=4, theta=0.5, bkt_fac=1, read_perc=0.5, use_sgx=True,
               cs=GB * 32, veri="PAGE_VERI", index="IDX_HASH", pre_load=1, use_log=0, txn_per_thd=10000,
               database_size=GB * 100, txn_length=64, enable_data_cache=True, pt=1, prof="false", wh=16,
               full_tpcc="false", nodes=1, test_freshness=0, veri_hash_buf_siz=KB * 4, real_time=0, sync_batch=16,
               vaccum=128, lazy_offloading=1, fast_chain=1, small_cs=False):
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
        "VERIFIED_CACHE_SIZ": int(cs / 2),
        "ENABLE_DATA_CACHE" : "true" if enable_data_cache else "false",
        "VERI_TYPE"			: veri,
        "INDEX_STRUCT"		: index,
        "PRE_LOAD"			: pre_load,
        "USE_LOG"			: use_log,
        "MAX_TXN_PER_PART"	: txn_per_thd,
        "SYNTH_TABLE_SIZE"	: int(database_size / 2 / KB), # 2kb per record
        "REQ_PER_QUERY"		: txn_length,
        "PART_CNT"          : pt,
        "PROFILING"         : prof,
        "NUM_WH"            : wh,
        "FULL_TPCC"         : full_tpcc,
        "NODE_CNT"          : nodes,
        "TEST_FRESHNESS"    : test_freshness,
        "MSG_SIZE_MAX"      : veri_hash_buf_siz,
        "REAL_TIME"         : real_time,
        "MSG_TIME_LIMIT"     : 0,
        "SYNC_VERSION_BATCH"   :sync_batch,
        "VACCUM_TRIGGER"        : vaccum,
        "LAZY_OFFLOADING"       : lazy_offloading,
        "FAST_VERI_CHAIN_ACCESS" : fast_chain,
        "SMALL_CACHE_SIZE"      : small_cs
    }


def test_compile(job):
    os.system("make clean> temp.out 2>&1")
    os.system("cp "+ dbms_cfg[0] +' ' + dbms_cfg[1])
    if job["USE_SGX"] == 1:
        if job["WORKLOAD"] == "TPCC":   # TODO: sgx pre release has bug.
            os.system("make sgx-debug 2>&1")
        else:
            os.system("make sgx-release 2>&1")
        pattern = r"<HeapMaxSize>.*</HeapMaxSize>"
        tp = max(min(job["SYNTH_TABLE_SIZE"]*2, job["VERIFIED_CACHE_SIZ"] *2*KB) * 4, 256* MB*4)
        # if job["SMALL_CACHE_SIZE"]:
        siz = hex(min(tp, 16* GB))
        # else:
        # siz = hex(tp)
        print(siz)
        replacement = "<HeapMaxSize>"+ siz + "</HeapMaxSize>"
        replace("trusted/Enclave.config.xml", pattern, replacement)
        pattern = r"<HeapInitSize>.*</HeapInitSize>"
        replacement = "<HeapInitSize>"+ siz + "</HeapInitSize>"
        replace("trusted/Enclave.config.xml", pattern, replacement)
        os.system("cat trusted/Enclave.config.xml")
    else:
        os.system("make no-sgx 2>&1")

    for (param, value) in job.items():
        pattern = r"\#define\s*" + re.escape(param) + r'.*'
        replacement = "#define " + param + ' ' + str(value)
        replace(dbms_cfg[1], pattern, replacement)

    # print("clean finished!!!!")
    os.system("rm -f storage/rocksdb/* 2>&1")
    time.sleep(0.5)
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
    if job["NODE_CNT"] == 1:
        cmd = "./App %s" % (app_flags)  # + fimeName
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        result = process.communicate()
        res = result[0].decode("utf-8")
        print(res)
        # print("input", res[res[0].find('[summary]'):])
        f.write(res[res.find('[summary]'):])
        process.wait()
        f.flush()
    else:
        cnt = job["NODE_CNT"]
        cmd = ["./App" for _ in range(cnt)]
        process = [None for _ in range(cnt)]
        for i in range(1, cnt):
            cmd[i] += " -nid%d -r100 -w0" % i
            print("running command ", cmd[i])
            process[i] = subprocess.Popen(cmd[i], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        time.sleep(1)   # wo node must begin before rw.
        cmd[0] += " -nid0"
        print("running command ", cmd[0])
        process[0] = subprocess.Popen(cmd[0], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)


        for i in range(cnt):
            result = process[i].communicate()
            res = result[0].decode("utf-8")
            print(res)
            # print("input", res[res[0].find('[summary]'):])
            f.write(res[res.find('[summary]'):])
            process[i].wait()
            f.flush()

    if job["USE_LOG"] == 1:
        process_store.kill()


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


# Small memory, local machine.
def run_thread_exp():
    global jobs
    jobs = OrderedDict()
    if not BigTest:
        for th in [1, 2, 3, 4, 5, 6, 7, 8]:
            insert_job("OCC", 'YCSB', thread_num=th, use_sgx=False)
            insert_job("OCC", 'YCSB', thread_num=th, use_sgx=True)
    else:
        for th in [1, 2, 3, 4, 5, 6, 7, 8]:
            insert_job("OCC", 'YCSB', thread_num=th, use_sgx=False, database_size=2*GB)
    run_all_test(jobs, "ycsb.thread.result")

# Local machine.
def run_tpc_exp():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        x_con = [1, 2, 3, 4, 5, 6, 7, 8]
        for th in x_con:
            for alg in algs:
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=False)
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, wh=4)
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=True)
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=True, wh=4)
    else:
        x_con = [1, 8]
        for th in x_con:
            for alg in algs:
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=False)
        for th in x_con:
            for alg in algs:
                insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, wh=4)

    run_all_test(jobs, "tpcc.thread.wh.result")

# Small memory, local machine.
def run_theta_exp():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        for th in [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]:
            insert_job("NO_WAIT", 'YCSB', theta=th, use_sgx=False)
            insert_job("NO_WAIT", 'YCSB', theta=th, use_sgx=True)
    else:
        for th in [0.0, 0.9]:
            insert_job("NO_WAIT", 'YCSB', theta=th, use_sgx=False, database_size=10*GB)
    run_all_test(jobs, "ycsb.theta.result")


# Small memory, local machine.
def run_rw_exp():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
            insert_job("NO_WAIT", 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=False)
            insert_job("NO_WAIT", 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=True)
    else:
        for th in [0.0, 1.0]:
            insert_job("NO_WAIT", 'YCSB', read_perc=th, thread_num=4, theta=0.9, use_sgx=False, database_size=10*GB)
    run_all_test(jobs, "ycsb.rw.result")


def run_common_test():
    global jobs
    jobs = OrderedDict()
    # insert_job("NO_WAIT", 'YCSB', use_sgx=False, database_size = 16*GB,
    #            small_cs=True, lazy_offloading=0, cs=128 * MB, txn_per_thd=1000) # No offloading
    insert_job("NO_WAIT", 'YCSB', use_sgx=False, database_size = 16*GB,
               small_cs=True, cs=128 * MB, txn_per_thd=1000) # No offloading
    run_all_test(jobs, "tmp.csv")


# Large memory, single node.
def run_cache_size_impact_for_different_methods_test():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        x_con = [128 * MB, 1 * GB, 8* GB, 16 * GB, 32 * GB]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, cs=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", cs=cs, pre_load=0)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", cs=cs, pre_load=0,
                       txn_per_thd=1000)
    else:
        x_con = [128 * MB, 32 * GB]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, cs=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, index="IDX_BTREE", cs=cs, pre_load=0)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, index="IDX_BTREE", veri="MERKLE_TREE", cs=cs, pre_load=0,
                       txn_per_thd=1000)
    run_all_test(jobs, "ycsb.cache.size.result")


# Large memory, single node.
def run_database_skew_test():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        x_con = [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, theta=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", theta=cs, pre_load=0)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", theta=cs, pre_load=0,
                       txn_per_thd=1000)
    else:
        x_con = [0.0,0.9]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, theta=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, index="IDX_BTREE", theta=cs, pre_load=0)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, index="IDX_BTREE", veri="MERKLE_TREE", theta=cs, pre_load=0,
                       txn_per_thd=1000)
    run_all_test(jobs, "ycsb.cache.skew.result")

# Large memory, single node.
def run_database_size_test():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        x_con = [1 * GB, 8 * GB, 32 * GB, 64 * GB, 100 * GB] #, 64 * GB, 100 * GB
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, database_size=cs, txn_per_thd=1000)
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", database_size=cs, pre_load=0, txn_per_thd=1000)
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", database_size=cs, pre_load=0,
                       txn_per_thd=1000)
    else:
        x_con = [32 * GB] #, 64 * GB
        # for cs in x_con:
        #     insert_job('NO_WAIT', 'YCSB', use_sgx=False, database_size=cs, txn_per_thd=1000)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", database_size=cs, pre_load=0, txn_per_thd=1000)
        # for cs in x_con:
        #     insert_job('NO_WAIT', 'YCSB', use_sgx=False, index="IDX_BTREE", veri="MERKLE_TREE", database_size=cs, pre_load=0,
        #                txn_per_thd=1000)

    run_all_test(jobs, "ycsb.cache.db.result")


def run_database_varying_txn_length():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        x_con = [1, 4, 16, 32, 64, 96, 128]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, txn_length=cs)
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, txn_length=cs)
            # insert_job('NO_WAIT', 'YCSB', use_sgx=False, txn_length=cs, read_perc=1)
            # insert_job('NO_WAIT', 'YCSB', use_sgx=True, txn_length=cs, read_perc=1)
    else:
        x_con = [1, 64]
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=False, txn_length=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, txn_length=cs)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", txn_length=cs, pre_load=0)
        for cs in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, index="IDX_BTREE", veri="MERKLE_TREE", txn_length=cs, pre_load=0,
                       txn_per_thd=1000)

    run_all_test(jobs, "ycsb.cache.txn_length.result")


def run_single_layer_cache_exp():
    global jobs
    jobs = OrderedDict()
    x_con = [(0.2, 1*GB), (0.5, 1*GB), (0.8, 1*GB), (0.2, 100*GB), (0.5, 100*GB), (0.8, 100*GB)]
    if BigTest:
        for (the, db_siz) in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, theta=the, database_size=db_siz)
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, theta=the, database_size=db_siz)
    else:
        # insert_job('NO_WAIT', 'YCSB', use_sgx=False, use_log=1, enable_data_cache=False, database_size=MB * 8, theta=0.9)
        insert_job('NO_WAIT', 'YCSB', use_sgx=False, use_log=1, lazy_offloading="false", theta=0.9, database_size=2*GB)
    run_all_test(jobs, "ycsb.single_layer.result")


def run_profiling():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 2, 3, 4, 5, 6, 7, 8]
    for th in x_con:
        insert_job("NO_WAIT", 'YCSB', thread_num=th, use_sgx=False, prof="true")
        insert_job("NO_WAIT", 'YCSB', thread_num=th, use_sgx=True, prof="true")
    run_all_test(jobs, "ycsb.profiling.result")


def run_full_tpcc_test():
    global jobs
    jobs = OrderedDict()
    if BigTest:
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=1, wh=4, txn_per_thd=1000)
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=4, wh=4, txn_per_thd=1000)
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=8, wh=4, txn_per_thd=1000)
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=1, wh=16, txn_per_thd=1000)
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=4, wh=16, txn_per_thd=1000)
        insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=8, wh=16, txn_per_thd=1000)
    else:
        insert_job("NO_WAIT", 'TPCC', use_sgx=False, full_tpcc="true", index="IDX_BTREE", thread_num=8, wh=16, txn_per_thd=1000)
    run_all_test(jobs, "tpcc.full.result")


def run_freshness_test():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 2, 4, 8, 16, 32]
    for x in x_con:
        insert_job("NO_WAIT", 'YCSB', use_sgx=False, nodes=2, test_freshness=1, sync_batch=x)
        # # for throughput
        # insert_job("NO_WAIT", 'YCSB', use_sgx=False, nodes=2, thread_num=1,
        #            txn_length=1, theta=99, test_freshness=1, sync_batch=x)
        # for freshness
    run_all_test(jobs, "freshness.log")

def run_test_vacuum():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 4, 16, 64, 256]
    for x in x_con:
        insert_job("NO_WAIT", 'YCSB', use_sgx=True, theta=0.5, nodes=2, sync_batch=1, vaccum=x, fast_chain=0)
        insert_job("NO_WAIT", 'YCSB', use_sgx=True, theta=0.8, nodes=2, sync_batch=1, vaccum=x, fast_chain=0)
    run_all_test(jobs, "vaccum.log")

def run_lazy_offloading():
    global jobs
    jobs = OrderedDict()
    # 1 * GB, 2*GB, 4*GB, 8* GB, 16 * GB, 32 * GB,
    x_con = [64 * GB, 80 * GB, 100 * GB]
    for siz in x_con:
        insert_job("NO_WAIT", 'YCSB', use_sgx=True, database_size = siz,small_cs=True, txn_per_thd=1000)
        insert_job("NO_WAIT", 'YCSB', use_sgx=True, cs=200*GB, database_size = siz, small_cs=True, txn_per_thd=1000) # No offloading
    run_all_test(jobs, "ycsb.cache.lazy.offloading")



# single node, small mem
run_thread_exp()
run_tpc_exp()
run_theta_exp()
# run_rw_exp()
run_profiling()
# run_common_test()
run_full_tpcc_test()

# single node, large mem
run_database_size_test()
run_cache_size_impact_for_different_methods_test()
run_database_skew_test()
run_database_varying_txn_length()
run_single_layer_cache_exp()

# RO and RW
# run_freshness_test()
# run_test_vacuum()
# run_lazy_offloading()

