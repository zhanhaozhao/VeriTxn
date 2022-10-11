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

def ycsb_scaling():
    wl = 'YCSB'
    #nnodes = [1,2,4,8,16,32,64]
    nnodes = [2]
    algos=['NO_WAIT']
    base_table_size=1048576*10
    write_perc = [0.0]
    load = [10000]
    tcnt = [4]
    ctcnt = [4]
    scnt = [2]
    rcnt = [2]
    skew = [0.0]
    #skew = [0.0,0.5,0.9]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT","CLIENT_THREAD_CNT","SEND_THREAD_CNT","REM_THREAD_CNT","CLIENT_SEND_THREAD_CNT","CLIENT_REM_THREAD_CNT"]
    exp = [[wl,n,algo,base_table_size*n,wr_perc,1-wr_perc,ld,sk,thr,cthr,sthr,rthr,sthr,rthr] for thr,cthr,sthr,rthr,wr_perc,sk,ld,n,algo in itertools.product(tcnt,ctcnt,scnt,rcnt,write_perc,skew,load,nnodes,algos)]
    return fmt,exp

def ycsb_skew():
    wl = 'YCSB'
    nnodes = [2]
    #algos=['WOOKONG','WAIT_DIE','MVCC','MAAT','TIMESTAMP','OCC']
    algos=['NO_WAIT']
    base_table_size=1048576*10
    write_perc = [0.5]
    load = [10000]
    tcnt = [4]
    skew = [0.0,0.25,0.5,0.55,0.6,0.65,0.7,0.75,0.8,0.9]
    # skew = [0.0,0.25,0.5,0.]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,n,algo,base_table_size*n,wr_perc,1-wr_perc,ld,sk,thr] for thr,wr_perc,ld,n,sk,algo in itertools.product(tcnt,write_perc,load,nnodes,skew,algos)]
    return fmt,exp

def ycsb_writes():
    wl = 'YCSB'
    nnodes = [1]
    algos=['NO_WAIT']
    base_table_size=2097152*8
    write_perc = [0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0]
    load = [1000]
    tcnt = [4]
    skew = [0.5]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","SYNTH_TABLE_SIZE","WRITE_PERC","READ_PERC","MAX_TXN_IN_FLIGHT","ZIPF_THETA","THREAD_CNT"]
    exp = [[wl,n,algo,base_table_size*n,wr_perc,1-wr_perc,ld,sk,thr] for thr,write_perc,ld,n,sk,algo in itertools.product(tcnt,write_perc,load,nnodes,skew,algos)]
    return fmt,exp


def tpcc_scaling():
    wl = 'TPCC'
    nnodes = [1]
    # nalgos=['NO_WAIT','WAIT_DIE','MAAT','MVCC','TIMESTAMP','CALVIN','WOOKONG']
    nalgos=['NO_WAIT','WAIT_DIE','MAAT','MVCC','TIMESTAMP','OCC','CALVIN','WOOKONG','TICTOC','DLI_DTA','DLI_DTA1','DLI_DTA2','DLI_DTA3','DLI_MVCC_OCC','DLI_MVCC']
    # nalgos=['WOOKONG']
    # nalgos=['NO_WAIT']
    npercpay=[0.0]
    # npercpay=[0.0]
    wh=128
    # wh=64
    load = [10000,20000,30000,40000,50000]
    tcnt = [100]
    ctcnt = [100]
    fmt = ["WORKLOAD","NODE_CNT","CC_ALG","PERC_PAYMENT","NUM_WH","MAX_TXN_IN_FLIGHT","THREAD_CNT","CLIENT_THREAD_CNT"]
    exp = [[wl,n,cc,pp,wh*n,tif,thr,cthr] for thr,cthr,tif,pp,n,cc in itertools.product(tcnt,ctcnt,load,npercpay,nnodes,nalgos)]

    # wh=4
    # exp = exp+[[wl,n,cc,pp,wh*n,tif] for tif,pp,n,cc in itertools.product(load,npercpay,nnodes,nalgos)]
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


##############################
# END PLOTS
##############################

experiment_map = {
    'ycsb_scaling': ycsb_scaling,
    'ycsb_writes': ycsb_writes,
    'ycsb_skew': ycsb_skew,
    'tpcc_scaling': tpcc_scaling,
    'tpcc_scaling_whset': tpcc_scaling_whset,
}


# Default values for variable configurations
configs = {
    "NODE_CNT" : 16,
    "THREAD_CNT": 4,
    "REM_THREAD_CNT": 1,
    "SEND_THREAD_CNT": 1,
    "MAX_TXN_PER_PART" : 10000,
    "WORKLOAD" : "YCSB",
    "CC_ALG" : "WAIT_DIE",
    "TPORT_PORT":"18000",
    "PART_CNT": "NODE_CNT",
    "PART_PER_TXN": 2,
    "MAX_TXN_IN_FLIGHT": 10000,
    "ABORT_PENALTY": "10 * 1000000UL   // in ns.",
    "ABORT_PENALTY_MAX": "5 * 100 * 1000000UL   // in ns.",
    "MSG_TIME_LIMIT": "0",
    "MSG_SIZE_MAX": 4096,
    "READ_PERC":0.0,
    "WRITE_PERC":0.0,
    "ISOLATION_LEVEL":"SERIALIZABLE",
#YCSB
    "INIT_PARALLELISM" : 8,
    "ZIPF_THETA":0.3,
    "DATA_PERC": 100,
    "REQ_PER_QUERY": 1,
    "SYNTH_TABLE_SIZE":"65536",
#TPCC
    "NUM_WH": 'PART_CNT',
    "PERC_PAYMENT":0.0
}

