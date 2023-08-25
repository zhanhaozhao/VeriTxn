VeriTxn
=======

VeriTxn is a cloud-native database that efficiently provides verifiability of transaction correctness. It relies on the trusted hardware (i.e., Intel SGX) to enable verifiable transaction processing. We use the codebase of [DBx1000](https://github.com/yxymit/DBx1000) as a starting point and extend it into a SGX-enabled cloud-native databases.

The following paper describes DBx1000: 

[1] Xiangyao Yu, George Bezerra, Andrew Pavlo, Srinivas Devadas, Michael Stonebraker, [Staring into the Abyss: An Evaluation of Concurrency Control with One Thousand Cores](http://www.vldb.org/pvldb/vol8/p209-yu.pdf), VLDB 2014


Dependencies
----------------------

The project is written primarily in C++ and was compiled with GNU Make using the GCC compiler. 
The two primary dependencies for the project are the jemalloc memory allocation library and the libnuma NUMA policy library. 

- Ubuntu 20.04
- Server equipped with Intel SGX
- Intel SGX SDK

- Rocksdb (≥ 5.8)


We run the experiments in a cluster of up to 8 nodes running Ubuntu 20.04 on Microsoft Azure.
Each node is a standard DC16s v3 server, equipped with an Intel(R) Platinum 8370C CPU at 2.8GHz (2 $\times$ 8 cores), with 128GB of DRAM. The EPC size is limited to 64GB.


Setup & Build
----------------------

### Edit IP Address

Edit the ifconfig.txt file. One line corresponds to one node.

### Build

#### Build without SGX

To build the database

    make no-sgx
    make clean && make -j

#### Build with SGX

To build the database using SGX (debug mode)

    make sgx-debug
    make clean && make -j

To build the database using SGX (prerelease mode)

    make sgx-release
    make clean && make -j


Run experiments
----------------------

    # edit the file "scripts/run_config.py" to setup the vcloud cluster for experiments, required changes:
    username: ssh username
    port: ssh port
    identity: public key for ssh auth
    vcloud_uname: source code folder path
    vcloud_machines: ips of nodes in the cluster. One line corresponds to one node.


### Run all the experiments

    # test as single node
    python test.py

    # test as a pure cloud-native database without SGX
    python run_experiments.py -e -r -ns -c vcloud <test_case_name>
    
    # test as a verifiable cloud-native database with SGX
    python run_experiments.py -e -r -c vcloud <test_case_name>


### Manually Edit the Configuration file

Configurations can be changed in the config.h file. 

    NODE_CNT          : Number of compute nodes modeled in the system
    THREAD_CNT        : Number of worker threads running in the compute node
    WORKLOAD          : Supported workloads include YCSB and TPCC
    MAX_TXN_PER_PART  : Number of transactions to run
    USE_SGX           : Enable verification or not
    VERIFIED_CACHE_SIZ: Size of verified cache
    ENABLE_DATA_CACHE : Enable data cache or not
    VERI_TYPE         : Verification method (MERKLE_TREE or PAGE_VERI)
    INDEX_STRUCT      : Index structure
    PROFILING         : Enable profiling or not
    REAL_TIME         : Enable real time stats or not


VeriTxn can be run in manual with

    # Set NODE_CNT to 1 to run in a single node mode
    ./App 


### Outputs

If run VeriTxn in manual, there are some metrics show in the console. Here we list several most important metrics:
- `txn_cnt`: The total number of committed transactions. This number is close to but smaller than THREAD_CNT * MAX_TXN_PER_PART. When any worker thread commits MAX_TXN_PER_PART transactions, all the other worker threads will be terminated.

- `abort_cnt`: The total number of aborted transactions. A transaction may abort multiple times before committing. Therefore, abort_cnt can be greater than txn_cnt.

- `run_time`: The aggregated transaction execution time (in seconds) across all threads. run_time is approximately the program execution time * THREAD_CNT. Therefore, the `per-thread throughput` is `txn_cnt / run_time` and the `total throughput` is `txn_cnt / run_time * THREAD_CNT`.

- `latency`: Average latency of transactions.


If you run the experiments, the output file is stored in ```results/<timestamp>```. You can check the `tmp-OCC` file for the results, where each line is the x-axis, RW node throughput, total throughput, abort rate, average latency, and RO node latency.



TPCC Specification
----------------------




show no impact of this configuration on performance, as the parsing overhead is negligible.
All these configurations are consistent with those detailed in our paper.



YCSB Specification
----------------------



Please refer to CONFIGURATION.md for the meaning of each configuration. Here we only list several most important ones. 