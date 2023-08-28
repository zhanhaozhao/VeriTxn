import itertools 
# from paper_plots import *
# Experiments to run and analyze
# Go to end of file to fill in experiments
SHORTNAMES = {
    "NODE_CNT" : "N",
    "THREAD_CNT" : "T",
    "REM_THREAD_CNT" : "RT",
    "SEND_THREAD_CNT" : "ST",
    "CC_ALG" : "",
    "WORKLOAD" : "",
    "MAX_TXN_PER_PART" : "TXNS",
    "MAX_TXN_IN_FLIGHT" : "TIF",
    "TXN_READ_PERC" : "RD",
    "TXN_WRITE_PERC" : "WR",
    "ZIPF_THETA" : "SKEW",
    "MSG_TIME_LIMIT" : "BT",
    "MSG_SIZE_MAX" : "BS",
    "DATA_PERC":"D",
    "PERC_PAYMENT":"PP",
    "REQ_PER_QUERY": "RPQ",
    "PRIORITY":"",
    "ABORT_PENALTY":"PENALTY",
    "SYNTH_TABLE_SIZE":"TBL",
    "NUM_WH":"WH",
}

fmt_title=["NODE_CNT","CC_ALG","WRITE_PERC","PERC_PAYMENT","MODE","MAX_TXN_IN_FLIGHT","SEND_THREAD_CNT","REM_THREAD_CNT","THREAD_CNT","ZIPF_THETA","NUM_WH"]

##############################
# PLOTS
##############################

def ycsb_thread():
    wl = 'YCSB'
    algos=['OCC']
    write_perc = [0.5]
    skew = [0.5]
    tcnt = [1,2,4,8,16]
    fmt = ["WORKLOAD","CC_ALG","WRITE_PERC","READ_PERC","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,algo,wr_perc,1-wr_perc,sk,thr] for algo,wr_perc,sk,thr in itertools.product(algos,write_perc,skew,tcnt)]
    return fmt,exp

def tpcc_thread():
    wl = 'TPCC'
    nalgos=['OCC']
    wh=16
    tcnt = [1,2,4,8,16]
    fmt = ["WORKLOAD","CC_ALG","NUM_WH","THREAD_CNT"]
    exp = [[wl,cc,wh,thr] for cc,thr in itertools.product(nalgos,tcnt)]
    return fmt,exp

def ycsb_scaling():
    wl = 'YCSB'
    nnodes = [2,3,4,5,6,7,8]
    algos=['OCC']
    write_perc = [0.5]
    tcnt = [4]
    skew = [0.5]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","WRITE_PERC","READ_PERC","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,n,algo,wr_perc,1-wr_perc,sk,thr] for thr,wr_perc,sk,n,algo in itertools.product(tcnt,write_perc,skew,nnodes,algos)]
    return fmt,exp

def ycsb_freshness():
    wl = 'YCSB'
    algos=['OCC']
    write_perc = [0.5]
    skew = [0.5]
    tcnt = [4]
    sync_batch= [1,2,4,8,16,32]
    fmt = ["WORKLOAD","CC_ALG","WRITE_PERC","READ_PERC","ZIPF_THETA","THREAD_CNT","SYNC_VERSION_BATCH"]
    exp = [[wl,algo,wr_perc,1-wr_perc,sk,thr,batch] for algo,wr_perc,sk,thr,batch in itertools.product(algos,write_perc,skew,tcnt,sync_batch)]
    return fmt,exp

def tpcc_wh():
    wl = 'TPCC'
    nalgos=['OCC']
    whs=[4,16]
    tcnt = [2,4,8]
    index = 'IDX_HASH'
    fulltpcc= 'false'
    cs = 1024*1024*1024*10
    fmt = ["WORKLOAD","CC_ALG","NUM_WH","THREAD_CNT","VERIFIED_CACHE_SIZ","INDEX_STRUCT","FULL_TPCC"]
    exp = [[wl,cc,wh,thr,cs,index,fulltpcc] for cc,thr,wh in itertools.product(nalgos,tcnt,whs)]
    return fmt,exp

# other experiments are performed on single RW node, please refer to test.py

def ycsb_varying_cache_size():
    wl = 'YCSB'
    nnodes = [1]
    algos=['OCC']
    base_table_size=1048576
    write_perc = [0.5]
    load = [10000]
    tcnt = [4]
    skew = [0.5]
    cache_size = [512*1024*1024, 1024*1024*1024, 2048*1024*1024, 4096*1024*1024, 8192*1024*1024]
    cache_algo = [[True,"PAGE_VERI","IDX_BTREE", 1]]
                  # [True,"PAGE_VERI","IDX_BTREE", 1],
                  # [True,"PAGE_VERI","IDX_BTREE", 0],
                  # [True,"MERKLE_TREE","IDX_BTREE", 0]]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC",
           "READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT",
           "CACHE_SIZE", "ENABLE_DATA_CACHE", "VERI_TYPE", "INDEX_STRUCT", "PRE_LOAD"]
    exp = [[wl,n,algo,base_table_size*n,write_perc,1-write_perc,ld,sk,thr,
            cs] for
           thr,write_perc,ld,n,sk,algo,cs in itertools.product(tcnt,write_perc,load,nnodes,skew,algos,cache_size)]
    print( "exp = ", exp)
    exps = []
    for a in exp:
        for b in cache_algo:
            exps.append(a+b)
    print( "exp = ", exps)
    return fmt,exps

def ycsb_fixed_cache_varying_database_size():
    wl = 'YCSB'
    nnodes = [1]
    algos=['OCC']
    base_table_size=[1, 8, 32, 128, 512, 1024, 2048] * 1024
    write_perc = [0.5]
    load = [10000]
    tcnt = [4]
    skew = [0.5]
    cache_size = [64] * 1024 * 1024
    cache_algo = [[True,"PAGE_VERI","IDX_HASH", 1],
                  [True,"PAGE_VERI","IDX_BTREE", 1],
                  [True,"PAGE_VERI","IDX_BTREE", 0],
                  [True,"MERKLE_TREE","IDX_BTREE", 0]]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC",
           "READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT",
           "CACHE_SIZE", "ENABLE_DATA_CACHE", "VERI_TYPE", "INDEX_STRUCT", "PRE_LOAD"]
    exp = [[wl,n,algo,base_table_size*n,write_perc,1-write_perc,ld,sk,thr,
            cs].append(ca) for
           thr,write_perc,ld,n,sk,algo,cs,ca in itertools.product(tcnt,write_perc,load,nnodes,skew,algos,cache_size,cache_algo)]
    return fmt,exp

def ycsb_fixed_cache_varying_skew():
    wl = 'YCSB'
    nnodes = [1]
    algos=['OCC']
    base_table_size=1048576
    write_perc = [0.5]
    load = [10000]
    tcnt = [4]
    skew = [0.1, 0.3, 0.5, 0.8, 0.9]
    cache_size = [64] * 1024 * 1024
    cache_algo = [[True,"PAGE_VERI","IDX_HASH", 1],
                  [True,"PAGE_VERI","IDX_BTREE", 1],
                  [True,"PAGE_VERI","IDX_BTREE", 0],
                  [True,"MERKLE_TREE","IDX_BTREE", 0]]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC",
           "READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT",
           "CACHE_SIZE", "ENABLE_DATA_CACHE", "VERI_TYPE", "INDEX_STRUCT", "PRE_LOAD"]
    exp = [[wl,n,algo,base_table_size*n,write_perc,1-write_perc,ld,sk,thr,
            cs].append(ca) for
           thr,write_perc,ld,n,sk,algo,cs,ca in itertools.product(tcnt,write_perc,load,nnodes,skew,algos,cache_size,cache_algo)]
    return fmt,exp

def ycsb_single_layer_cache():
    wl = 'YCSB'
    nnodes = [1]
    algos=['OCC']
    # algos=['NO_WAIT']
    # base_table_size=1048576*10
    base_table_size=1048576
    write_perc = [0.5]
    load = [10000]
    tcnt = [4]
    skew = [0.6]
    data_cache = 'false'
    use_log = 1
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT", "ENABLE_DATA_CACHE", "USE_LOG"]
    exp = [[wl,n,algo,base_table_size,wr_perc,1-wr_perc,ld,sk,thr, data_cache, use_log] for thr,wr_perc,ld,n,sk,algo in itertools.product(tcnt,write_perc,load,nnodes,skew,algos)]
    return fmt,exp

def tpcc_scaling_whset():
    wl = 'TPCC'
    nnodes = [1,2,4,8,16,32,64]
    nalgos=['NO_WAIT','WAIT_DIE','MAAT','MVCC','TIMESTAMP','CALVIN','WOOKONG']
    npercpay=[0.0,0.5,1.0]
    wh=128
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","PERC_PAYMENT","NUM_WH"]
    exp = [[wl,n,cc,pp,wh] for pp,n,cc in itertools.product(npercpay,nnodes,nalgos)]
    wh=256
    exp = exp + [[wl,n,cc,pp,wh] for pp,n,cc in itertools.product(npercpay,nnodes,nalgos)]
    return fmt,exp


def ycsb_skew():
    wl = 'YCSB'
    nnodes = [4]
    #algos=['WOOKONG','WAIT_DIE','MVCC','MAAT','TIMESTAMP','OCC']
    algos=['NO_WAIT']
    # base_table_size=1048576*10
    base_table_size=1048576
    write_perc = [0.5]
    load = [10000]
    tcnt = [1]
    skew = [0.0,0.25,0.5,0.55,0.6,0.65,0.7,0.75,0.8,0.9]
    # skew = [0.1,0.3,0.5,0.7,0.9,1.1,1.3,1.5,1.7,1.9]
    # skew = [0.1,0.3,0.5,0.7,0.9]
    # skew = [0.1]
    # skew = [0.6]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,n,algo,base_table_size*n,wr_perc,1-wr_perc,ld,sk,thr] for thr,wr_perc,ld,n,sk,algo in itertools.product(tcnt,write_perc,load,nnodes,skew,algos)]
    return fmt,exp

def ycsb_writes():
    wl = 'YCSB'
    nnodes = [2]
    algos=['NO_WAIT']
    base_table_size=1048576
    write_perc = [0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0]
    # write_perc = [0.4,0.5,0.6,0.7]
    load = [10000]
    tcnt = [4]
    skew = [0.5]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,n,algo,base_table_size*n,write_perc,1-write_perc,ld,sk,thr]
           for thr,write_perc,ld,n,sk,algo in itertools.product(tcnt,write_perc,load,nnodes,skew,algos)]
    return fmt,exp


##############################
# END PLOTS
##############################

experiment_map = {
    'ycsb_thread': ycsb_thread,
    'ycsb_scaling': ycsb_scaling,
    'ycsb_writes': ycsb_writes,
    'ycsb_freshness': ycsb_freshness,
    'ycsb_skew': ycsb_skew,
    'tpcc_thread': tpcc_thread,
    'tpcc_wh': tpcc_wh,
    'tpcc_scaling_whset': tpcc_scaling_whset,
    'ycsb_varying_cache_size': ycsb_varying_cache_size,
    'ycsb_fixed_cache_varying_database_size': ycsb_fixed_cache_varying_database_size,
    'ycsb_fixed_cache_varying_skew': ycsb_fixed_cache_varying_skew,
    'ycsb_single_layer_cache': ycsb_single_layer_cache
}


# Default values for variable configurations
configs = {
    "NODE_CNT" : 4,
    "THREAD_CNT": 4, 
    "REM_THREAD_CNT": 1,
    "SEND_THREAD_CNT": 1,
    "MAX_TXN_PER_PART" : 10000, 
    "WORKLOAD" : "YCSB", 
    "CC_ALG" : "OCC", 
    "TPORT_PORT":"18000",
    "ABORT_PENALTY": "10 * 1000000UL   // in ns.",
    "MSG_TIME_LIMIT": "0", 
    "MSG_SIZE_MAX": 4096,
    "ISOLATION_LEVEL":"SERIALIZABLE",
    "USE_LOG": 1,
    "ENABLE_DATA_CACHE": "true",
    "BUCKET_FACTOR": 1,
    "USE_SGX": 1,
    "VERIFIED_CACHE_SIZ": 1024*1024*1024*4,
    "DATA_CACHE_SIZE": 1024*1024*1024*4,
    "VERI_TYPE"	: "PAGE_VERI",
    "INDEX_STRUCT" : "IDX_BTREE",
    "PRE_LOAD"	: 1,
    "PROFILING" : "false",
    "MSG_SIZE_MAX" : 4 * 1024,
    "REAL_TIME"  : 0,
    "SYNC_VERSION_BATCH"  : 16,
    "VACCUM_TRIGGER"        : 128,
    "LAZY_OFFLOADING"       : 1,
    "FAST_VERI_CHAIN_ACCESS" : 1,
    "SMALL_CACHE_SIZE"      : False,
    "VERI_BATCH": 1 * 1000000000,
    "TAMPER_PERCENTAGE" : 0,
    "TAMPER_RECOVERY" : 1,
#YCSB
    "INIT_PARALLELISM": 8,
    "ZIPF_THETA":0.5,
    "READ_PERC": 0.5,
    "WRITE_PERC": 0.5,
    "REQ_PER_QUERY": 16,
    "SYNTH_TABLE_SIZE": 10*1000*1000,
#TPCC
    "NUM_WH": 16,
    "FULL_TPCC" : "true",
    "TPCC_ACCESS_ALL" : "false",
    "PERC_PAYMENT":0.5
}