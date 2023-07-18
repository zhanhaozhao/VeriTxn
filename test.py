import os, sys, re, os.path
import platform
import string
import subprocess, datetime, time, signal

from collections import defaultdict, OrderedDict

UseSGX = False
BigTest = True
CompileOnly = False
KB = 1024
MB = 1024 * KB
GB = 1024 * MB
ReadOnly = False
max_siz_per_record = 3 * KB / 2
BILLION = 1000000000

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
               cs=GB * 4, ds=GB * 16, veri="PAGE_VERI", index="IDX_HASH", pre_load=1, use_log=0, txn_per_thd=10000,
               table_size=10 * MB, txn_length=16, enable_data_cache=True, pt=1, prof="false", wh=16,
               full_tpcc="false", nodes=1, test_freshness=0, veri_hash_buf_siz=KB * 4, real_time=0, sync_batch=16,
               vaccum=128, lazy_offloading=1, fast_chain=1, small_cs=False, veri_batch_sec = 1, tamper = 0, tamper_interval = BILLION):
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
        "DATA_CACHE_SIZE" : ds,
        "TAMPER_PERCENTAGE" : tamper,
        "TAMPER_INTERVAL" : tamper_interval,
        "ENABLE_DATA_CACHE" : "true" if enable_data_cache else "false",
        "VERI_TYPE"			: veri,
        "INDEX_STRUCT"		: index,
        "PRE_LOAD"			: pre_load,
        "USE_LOG"			: use_log,
        "MAX_TXN_PER_PART"	: txn_per_thd,
        "SYNTH_TABLE_SIZE"	: table_size,
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
        "SMALL_CACHE_SIZE"      : small_cs,
        "VERI_BATCH": veri_batch_sec * 1000000000,
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
        tp = max(job["SYNTH_TABLE_SIZE"] * max_siz_per_record * 4, 1*GB)
        # if job["SMALL_CACHE_SIZE"]:
        print(tp)
        siz = hex(min(tp, 28 * GB * 4))
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
        process_store = subprocess.Popen("./Store", stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        # process_store = subprocess.Popen(["./Store", ">./store.log"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(5)
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
        # exit(0)
        pid = process_store.pid
        os.kill(pid, 9)
        # process_store.kill()
        process_store.wait()
    # exit(0)
    #     pid = process_store.pid
    #     os.kill(pid, 9)
    #     print("we are here")
    #     process_store.wait()


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
            if CompileOnly:
                exit(0)
            test_run(job, f)
            os.system("make clean> temp.out 2>&1")
    f.close()


# Small memory, local machine.
def run_thread_exp():
    global jobs
    jobs = OrderedDict()
    for th in [1, 2, 3, 4, 5, 6, 7, 8]:
        insert_job("OCC", 'YCSB', thread_num=th, use_sgx=False)
        insert_job("OCC", 'YCSB', thread_num=th, use_sgx=True)
    run_all_test(jobs, "ycsb.thread.result")

def run_common_exp():
    global jobs
    jobs = OrderedDict()
    # insert_job("NO_WAIT", 'YCSB', use_sgx=False, table_size=1*MB) #1.398 GB
    # insert_job("NO_WAIT", 'YCSB', use_sgx=False, table_size=10*MB) #12.76 GB
    insert_job("NO_WAIT", 'YCSB', use_sgx=True, table_size=32*MB) #10.162 GB in side s
    insert_job("NO_WAIT", 'YCSB', use_sgx=True, table_size=48*MB) #10.162 GB in side s
        # insert_job("OCC", 'YCSB', thread_num=th, use_sgx=True)
    run_all_test(jobs, "ycsb.thread.result")


# Local machine.
def run_tpc_exp():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 2, 3, 4, 5, 6, 7, 8]
    for th in x_con:
        for alg in algs:
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=False)
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=False, wh=4)
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=True)
            insert_job(alg, 'TPCC', thread_num=th, use_sgx=True, wh=4)


    run_all_test(jobs, "tpcc.thread.wh.result")

# Small memory, local machine.
def run_theta_exp():
    global jobs
    jobs = OrderedDict()
    for th in [0.0, 0.3, 0.5, 0.6, 0.8, 0.9]:
        insert_job("NO_WAIT", 'YCSB', theta=th, use_sgx=False)
        insert_job("NO_WAIT", 'YCSB', theta=th, use_sgx=True)
    run_all_test(jobs, "ycsb.theta.result")


# Small memory, local machine.
def run_rw_exp():
    global jobs
    jobs = OrderedDict()
    for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
        insert_job("NO_WAIT", 'YCSB', read_perc=th, use_sgx=False)
        insert_job("NO_WAIT", 'YCSB', read_perc=th, use_sgx=True)
    run_all_test(jobs, "ycsb.rw.result")


# def run_common_test():
#     global jobs
#     jobs = OrderedDict()
#     # insert_job("NO_WAIT", 'YCSB', use_sgx=False, database_size = 16*GB,
#     #            small_cs=True, lazy_offloading=0, cs=128 * MB, txn_per_thd=1000) # No offloading
#     insert_job("NO_WAIT", 'YCSB', use_sgx=False, database_size = 16*GB,
#                small_cs=True, cs=128 * MB, txn_per_thd=1000) # No offloading
#     run_all_test(jobs, "tmp.csv")


# Large memory, single node.

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

def run_cache_size_impact_for_different_methods_test():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    # x_con = [512 * MB, 1 * GB, 2 * GB, 4 * GB, 8 * GB]
    x_con = [4 * GB, 4 * GB, 4 * GB, 8 * GB, 8 * GB, 8 * GB]
    for cs in x_con:
        # insert_job(index="IDX_BTREE", cs=cs)
        insert_job(index="IDX_BTREE", veri="MERKLE_TREE", cs=cs)
        # insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", cs=cs)
        # insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", cs=cs, pre_load=0)
    run_all_test(jobs, "ycsb.cache.size.result")

def run_heatmap():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    # x_con = [512 * MB, 1 * GB, 2 * GB, 4 * GB, 8 * GB]
    # veri_cache = [GB / 2, 1 * GB, GB + GB / 2, 2 * GB, 2 * GB + GB / 2, 3 * GB,  3 * GB + GB / 2,
    #               4 * GB, 4 * GB + GB/2, 5 * GB, 5 * GB + GB/2, 6 * GB]
    # veri_cache = [2* GB, 4* GB, 6* GB, 8* GB, 10* GB, 12* GB, 14* GB, 16* GB, 18* GB, 20* GB]
    veri_cache = [18* GB, 20* GB, 18* GB, 20* GB, 18* GB, 20* GB, 18* GB, 20* GB]
    # veri_cache = [4 * GB]
    # data_cache = [GB / 2, 1 * GB, GB + GB / 2, 2 * GB, 2 * GB + GB / 2, 3 * GB,  3 * GB + GB / 2,
    #               4 * GB, 4 * GB + GB/2, 5 * GB, 5 * GB + GB/2, 6 * GB]
    data_cache = [20* GB]
    # varying data cache size.
    for cs in veri_cache:
        for ds in data_cache:
            # if cs == 16 * GB and (ds < 10 *GB or ds == 16*GB):
            #     continue
            # if cs != 14 * GB and (cs + ds > 20 * GB):
            #     continue
            # if cs + ds > 22 * GB:
            #     continue
            insert_job(table_size=20 * MB, cs=cs, ds=ds, use_log=1)
            # if cs == 1 * GB and ds == 16 *GB:
                # insert_job(table_size=4 * MB, cs=cs, ds=ds, use_log=1)
    run_all_test(jobs, "ycsb.cache.heatmap.result")

def run_tamper():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    tamper =  [1, 2, 4, 8, 16]
    for tp in tamper:
        insert_job(tamper=tp, use_log=1)
    run_all_test(jobs, "ycsb.cache.tamper.result")

def run_tamper_interval():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    #  1 * BILLION, 4 * BILLION
    tamper =  [1, 2 * BILLION, 4 * BILLION, 6 * BILLION, 8 * BILLION, 10 * BILLION, 12 * BILLION, 14 * BILLION, 16 * BILLION]
    # , 4 * BILLION, 16 * BILLION, 32 * BILLION, 64 * BILLION
    for tp in tamper:
        insert_job(cs = 1 * GB, tamper=20, tamper_interval=tp, use_log=1)
    # for tp in tamper:
    #     insert_job(cs = 1 * GB, tamper=20, tamper_interval=tp, use_log=1)
    run_all_test(jobs, "ycsb.cache.interval.result")

# Large memory, single node.
def run_database_size_test():
    global jobs, CompileOnly, max_siz_per_record
    # CompileOnly = True
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [1 * MB,  2 * MB, 4 * MB, 8 * MB, 16 * MB]
    # for cs in x_con:
    #     insert_job(index="IDX_BTREE", veri="PAGE_VERI", table_size=cs)
    # for cs in x_con:
    #     insert_job(index="IDX_BTREE", veri="MERKLE_TREE", table_size=cs)
    for cs in x_con:
        insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", table_size=cs)
    run_all_test(jobs, "ycsb.cache.db.result")

def run_database_c_size_test():
    global jobs, CompileOnly, max_siz_per_record
    # CompileOnly = True
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [1 * MB,  2 * MB, 4 * MB, 8 * MB, 16 * MB]
    # x_con = [4 * MB]
    for cs in x_con:
        # insert_job(index="IDX_BTREE", table_size=cs)
    # for cs in x_con:
    #     insert_job(index="IDX_BTREE", veri="MERKLE_TREE", table_size=cs)
    # for cs in x_con:
        insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", table_size=cs)
        # insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", table_size=cs, enable_data_cache=False)
    run_all_test(jobs, "ycsb.cache.db.result")

def run_database_varying_txn_length():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 4, 16, 32]
    for ll in x_con:
        # insert_job(index="IDX_BTREE", txn_length=ll, use_sgx=False, cs=16 * GB) # unlimited baseline.
        # insert_job(index="IDX_BTREE", txn_length=ll)
        # insert_job(index="IDX_BTREE", veri="MERKLE_TREE", txn_length=ll)
        insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", txn_length=ll)
        insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", txn_length=ll, enable_data_cache=False)
    run_all_test(jobs, "ycsb.cache.txn_length.result")

def run_single_layer_cache_exp():
    global jobs
    jobs = OrderedDict()
    x_con = [(0.2, 64*KB), (0.5, 64*KB), (0.8, 64*KB), (0.2, 16*MB), (0.5, 16*MB), (0.8, 16*MB)]
    if BigTest:
        for (the, db_siz) in x_con:
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, enable_data_cache=False, theta=the, table_size=db_siz)
            insert_job('NO_WAIT', 'YCSB', use_sgx=True, use_log=1, theta=the, table_size=db_siz)
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
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=1, wh=4, txn_per_thd=1000)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=4, wh=4, txn_per_thd=1000)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=8, wh=4, txn_per_thd=1000)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=1, wh=16, txn_per_thd=1000)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=4, wh=16, txn_per_thd=1000)
    insert_job("NO_WAIT", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", thread_num=8, wh=16, txn_per_thd=1000)
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

def run_common_test():
    global jobs, CompileOnly, max_siz_per_record
    # CompileOnly = True
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [16 * MB]
    for cs in x_con:
        insert_job(index="IDX_BTREE", veri="DEFERRED_MEMORY", table_size=cs)
        #  enable_data_cache=False
    run_all_test(jobs, "common.result")

# profiling_4_btree()
# single node, small mem
# run_thread_exp()
# run_tpc_exp()
# run_theta_exp()
# run_rw_exp()
# run_profiling()
# run_common_test()
# run_full_tpcc_test()

# single node, large mem
# run_database_c_size_test()
# run_tamper()
run_tamper_interval()
# run_heatmap()
# # run_database_size_test()
# run_database_varying_txn_length()
# run_single_layer_cache_exp()
# run_database_skew_test()

# RO and RW
# run_freshness_test()
# run_test_vacuum()
# run_lazy_offloading()

