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
algs = ['OCC']
count_job = 0

# insert_job insert given setting into jobs.
def insert_job(alg="OCC", workload="YCSB", thread_num=4, theta=0.5, bkt_fac=1, read_perc=0.5, use_sgx=True,
               cs=GB * 4, ds=GB * 4, veri="PAGE_VERI", index="IDX_BTREE", use_log=1, txn_per_thd=10000,
               table_size=10 * 1000 * 1000, txn_length=16, enable_data_cache=True, pt=1, prof="false", wh=16,
               full_tpcc="true", nodes=1, test_freshness=0, veri_hash_buf_siz=KB * 4, real_time=0, sync_batch=16,
               vaccum=128, lazy_offloading=1, fast_chain=1, small_cs=False, veri_batch_sec = 1, tamper = 0, tamper_interval = 1,
               tamper_recovery = 1, access_all=False):
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
        "TAMPER_RECOVERY" : tamper_recovery,
        "TPCC_ACCESS_ALL" : "true" if access_all else "false",
        "ENABLE_DATA_CACHE" : "true" if enable_data_cache else "false",
        "VERI_TYPE"			: veri,
        "INDEX_STRUCT"		: index,
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
        "PERC_PAYMENT" : 0.5,
    }

# test_compile update the config.h according to current setting, build the binaries.
def test_compile(job):
    os.system("make clean> temp.out 2>&1")
    os.system("cp "+ dbms_cfg[0] +' ' + dbms_cfg[1])
    if job["USE_SGX"] == 1:
        if job["WORKLOAD"] == "TPCC" and job["FULL_TPCC"] == "false":
            os.system("make sgx-debug 2>&1")
        else:
            os.system("make sgx-release 2>&1")
        pattern = r"<HeapMaxSize>.*</HeapMaxSize>"
        tp = max(job["SYNTH_TABLE_SIZE"] * max_siz_per_record * 4, 1*GB)
        # if job["SMALL_CACHE_SIZE"]:
        print(tp)
        siz = hex(min(tp, 32 * GB))
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

# test_run run an experiment with settings in job.
def test_run(job, f, test=''):
    global process_store
    print(job)
    app_flags = ""  # m_txn->run_txn
    if test == 'read_write':
        app_flags = "-Ar -t1"
    if test == 'conflict':
        app_flags = "-Ac -t4"

    if job["USE_LOG"] == 1:
        # start storage.
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

# run_all_test run tests in jobs one by one and output the result into filename.
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


def run_thread_exp():
    global jobs
    jobs = OrderedDict()
    for th in [1, 2, 4, 6, 8, 10]:
        insert_job("OCC", 'YCSB', index="IDX_HASH", thread_num=th, use_sgx=False, cs=10*GB, use_log=0)
        insert_job("OCC", 'YCSB', index="IDX_HASH", thread_num=th, use_sgx=True, cs=10*GB, use_log=0)
    run_all_test(jobs, "ycsb.thread.result")

def run_tpc_exp():
    global jobs, CompileOnly
    jobs = OrderedDict()
    # CompileOnly = True
    x_con = [2, 4, 8]
    for th in x_con:
        for alg in algs:
            insert_job(alg, 'TPCC', index="IDX_HASH", thread_num=th, use_sgx=True, wh=4, full_tpcc="false", cs=10*GB, table_size=10*MB, use_log=0)
            insert_job(alg, 'TPCC', index="IDX_HASH", thread_num=th, use_sgx=True, full_tpcc="false", cs=10*GB, table_size=10*MB, use_log=0)
            insert_job(alg, 'TPCC', index="IDX_HASH", thread_num=th, use_sgx=True, wh=4, full_tpcc="false", cs=10*GB, table_size=10*MB, use_log=0)
            insert_job(alg, 'TPCC', index="IDX_HASH", thread_num=th, use_sgx=True, full_tpcc="false", cs=10*GB, table_size=10*MB, use_log=0)
    run_all_test(jobs, "tpcc.thread.wh.result")

def run_theta_exp():
    global jobs
    jobs = OrderedDict()
    for th in [0.0, 0.3, 0.5, 0.6, 0.7, 0.8, 0.9]:
        insert_job("OCC", 'YCSB',index="IDX_HASH", theta=th, use_sgx=False, cs=10*GB, use_log=0)
        insert_job("OCC", 'YCSB',index="IDX_HASH", theta=th, use_sgx=True, cs=10*GB, use_log=0)
    run_all_test(jobs, "ycsb.theta.result")


def run_cache_size_impact_for_different_methods_test():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [512 * MB, 1 * GB, 2 * GB, 4*GB, 8*GB]
    for cs in x_con:
        insert_job(cs=cs)
        insert_job(veri="MERKLE_TREE", cs=cs)
        insert_job(veri="DEFERRED_MEMORY", cs=cs)
    for cs in x_con:
        insert_job(cs=cs, txn_length=1)
        insert_job(veri="MERKLE_TREE", cs=cs, txn_length=1)
        insert_job(veri="DEFERRED_MEMORY", cs=cs, txn_length=1)
        insert_job(veri="DEFERRED_MEMORY", cs=cs, txn_length=1, veri_batch_sec=100)
    run_all_test(jobs, "ycsb.cache.size.result")

def run_heatmap():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    veri_cache = [1 * GB, 2 * GB, 3 *GB, 4*GB, 5*GB, 6*GB, 7*GB, 8*GB, 9*GB, 10*GB]
    data_cache = [1 * GB, 2 * GB, 3 *GB, 4*GB, 5*GB, 6*GB, 7*GB, 8*GB, 9*GB, 10*GB]
    for cs in veri_cache:
        for ds in data_cache:
            insert_job(cs=cs, ds=ds)
    run_all_test(jobs, "ycsb.cache.heatmap.result")

def run_tamper():
    global jobs, max_siz_per_record
    max_siz_per_record = KB
    jobs = OrderedDict()
    tamper =  [1, 2, 4, 8, 16]
    for tp in tamper:
        insert_job(tamper=tp)
    run_all_test(jobs, "ycsb.cache.tamper.result")


def run_database_size_test():
    global jobs, CompileOnly, max_siz_per_record
    # CompileOnly = True
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [1 * MB,  2 * MB, 4 * MB, 8 * MB, 16 * MB]
    for cs in x_con:
        insert_job(veri="PAGE_VERI", table_size=cs)
        insert_job(veri="MERKLE_TREE", table_size=cs)
        insert_job(veri="DEFERRED_MEMORY", table_size=cs)
    run_all_test(jobs, "ycsb.cache.db.result")

def run_database_c_size_test():
    global jobs, CompileOnly, max_siz_per_record
    # CompileOnly = True
    max_siz_per_record = KB
    jobs = OrderedDict()
    x_con = [1 * MB,  2 * MB, 4 * MB, 8 * MB, 16 * MB]
    for cs in x_con:
        insert_job(table_size=cs)
        insert_job(veri="MERKLE_TREE", table_size=cs)
        insert_job(veri="DEFERRED_MEMORY", table_size=cs)
    run_all_test(jobs, "ycsb.cache.db.result")

def run_database_varying_txn_length():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 4, 16, 32, 64]
    for ll in x_con:
        insert_job(txn_length=ll, use_sgx=False, cs=16 * GB) # unlimited baseline.
        insert_job(txn_length=ll)
        insert_job(veri="MERKLE_TREE", txn_length=ll)
        insert_job(veri="DEFERRED_MEMORY", txn_length=ll)
    run_all_test(jobs, "ycsb.cache.txn_length.result")

def run_profiling():
    global jobs
    jobs = OrderedDict()
    x_con = [1, 2, 4, 6, 8, 10]
    for th in x_con:
        insert_job("OCC", 'YCSB', thread_num=th, index="IDX_HASH", use_sgx=False, prof="true", cs=10*GB, use_log=0)
        insert_job("OCC", 'YCSB', thread_num=th, index="IDX_HASH", use_sgx=True, prof="true", cs=10*GB, use_log=0)
    run_all_test(jobs, "ycsb.profiling.result")


def run_full_tpcc_test():
    global jobs, CompileOnly
    thrs = [1, 2, 4, 8, 16]
    for tr in thrs:
        insert_job("OCC", 'TPCC', use_sgx=True, full_tpcc="true", index="IDX_BTREE", wh=16, thread_num=tr)
    run_all_test(jobs, "tpcc.full.result")


# cache size 
run_cache_size_impact_for_different_methods_test()

# compare with litmus
run_thread_exp()
run_profiling()
run_theta_exp()
run_tpc_exp()

# full tpcc
run_full_tpcc_test()

# for revision response letter
# run_tamper()
# run_heatmap()