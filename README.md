VeriTxn
=======

VeriTxn is a cloud-native database that efficiently provides verifiability of transaction correctness. It relies on the trusted hardware (i.e., Intel SGX) to enable verifiable transaction processing. We implemented VeriTxn based on the codebase of [DBx1000](https://github.com/yxymit/DBx1000). 

The following paper describes DBx1000: 

[1] Xiangyao Yu, George Bezerra, Andrew Pavlo, Srinivas Devadas, Michael Stonebraker, [Staring into the Abyss: An Evaluation of Concurrency Control with One Thousand Cores](http://www.vldb.org/pvldb/vol8/p209-yu.pdf), VLDB 2014


Dependencies
----------------------

- Sever equipped with Intel SGX
- Intel SGX SDK


Setup & Build
----------------------

### Edit IP Address

Edit the ifconfig.txt file. One line corresponds to one node.

### Edit the Configuration file

Configurations can be changed in the config.h file. Please refer to README for the meaning of each configuration. Here we only list several most important ones. 

    NODE_CNT          : Number of compute nodes modeled in the system
    THREAD_CNT        : Number of worker threads running in the compute node
    WORKLOAD          : Supported workloads include YCSB and TPCC
    MAX_TXN_PER_PART  : Number of transactions to run per thread per partition
    USE_SGX           : Enable verification or not

### Build

To build the database

    make clean && make -j


Run & Test
----------------------

VeriTxn can be run in manual with

    # Set NODE_CNT to 1 to run in a single node mode
    ./App 

To run the experiments

    # test as a pure cloud-native database without SGX
    python run_experiments.py -e -r -ns -c vcloud <test_case_name>
    
    # test as a verifiable cloud-native database with SGX
    python run_experiments.py -e -r -c vcloud <test_case_name>


Outputs
----------------------

If run VeriTxn in manual, there are some metrics show in the console. Here we list several most important metrics:
- `txn_cnt`: The total number of committed transactions. This number is close to but smaller than THREAD_CNT * MAX_TXN_PER_PART. When any worker thread commits MAX_TXN_PER_PART transactions, all the other worker threads will be terminated.

- `abort_cnt`: The total number of aborted transactions. A transaction may abort multiple times before committing. Therefore, abort_cnt can be greater than txn_cnt.

- `run_time`: The aggregated transaction execution time (in seconds) across all threads. run_time is approximately the program execution time * THREAD_CNT. Therefore, the `per-thread throughput` is `txn_cnt / run_time` and the `total throughput` is `txn_cnt / run_time * THREAD_CNT`.

- `latency`: Average latency of transactions.


If you run the experiments, the output file is stored in ```results/<timestamp>```. You can check the `tmp-OCC` file for the results, where each line is the x-axis, RW node throughput, total throughput, abort rate, average latency, and RO node latency.

