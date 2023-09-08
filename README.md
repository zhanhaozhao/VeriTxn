VeriTxn
=======

VeriTxn is a cloud-native database that efficiently provides verifiability of transaction correctness. It leverages the trusted hardware (i.e., Intel SGX) to enable verifiable transaction processing. We use the codebase of [DBx1000](https://github.com/yxymit/DBx1000) as our starting point and extend it to become an SGX-enabled cloud-native database.

The following paper describes DBx1000: 

[1] Xiangyao Yu, George Bezerra, Andrew Pavlo, Srinivas Devadas, Michael Stonebraker, [Staring into the Abyss: An Evaluation of Concurrency Control with One Thousand Cores](http://www.vldb.org/pvldb/vol8/p209-yu.pdf), VLDB 2014


Dependencies
----------------------

The project is written primarily in C++ and is compiled with GNU Make using the GCC compiler. 
The primary dependencies for the project are:

- Server equipped with Intel SGX
- Ubuntu 20.04
- Intel SGX SDK (≥ 2.14)
- GCC (≥ 7.5)
- Jemalloc (≥ 3.0)
- Rocksdb (≥ 5.8)
- Libprotoc (≥ 3.6.1)
- Grpc (≥ 1.33)
- Nanomsg (≥ 1.0.0)


Build
----------------------

Note: Our test scripts will automatically build the code, making this step optional. However, we highly recommend trying this manual build before running any scripts to ensure all dependencies are correctly installed.

### Build without SGX

To build the VeriTxn (w/o verification)

    make no-sgx
    make clean && make -j

### Build with SGX

To build the database using SGX (debug mode)

    make sgx-debug
    make clean && make -j

To build the database using SGX (prerelease mode)

    make sgx-release
    make clean && make -j


Run Experiments
----------------------

To run the experiments mentioned in our paper, follow these two steps:

1. Register the server information: Begin by registering the servers used in the experiments.
2. Execute the scripts: Once registration is complete, run the provided scripts to conduct the experiments.

### Register the server information

Edit the file `script/run_config.py` to setup the server cluster for experiments. Please make the following required changes:

```
username: [Your SSH username]
port: [Your SSH port number]
identity: [Path to your public key for SSH authentication]
vcloud_uname: [Path to the source code folder]
vcloud_machines: [List of IP addresses for nodes in the cluster]
```


*Warning*: Our experiments were conducted on a cluster of up to 8 nodes running Ubuntu 20.04 on Microsoft Azure.
Each node is a standard DC16s v3 server, equipped with an Intel(R) Platinum 8370C CPU at 2.8GHz (16 cores), with 128GB of DRAM. The EPC size is limited to 64GB. Please note that if you use different hardware or software configurations, the exact performance outcomes may differ from those presented in our paper.



### Run all experiments

We have provided a comprehensive set of scripts to assist you in running the experiments corresponding to the figures in our paper.

To run these experiments, please use `script/run_experiments.py`:

```
cd script

# test as VeriTxn (w/o verification)
python run_experiments.py -e -r -ns -c vcloud <test_case_name>

# test as VeriTxn
python run_experiments.py -e -r -c vcloud <test_case_name>
```

For the experiments in Figure 6, the available `<test_case_name>` options are:
- `ycsb_thread`: vary thread counts on YCSB workload (Figure 6a);
- `tpcc_thread`: vary thread counts on TPCC workload (Figure 6b);
- `ycsb_scaling`: vary the number of nodes on YCSB workload (Figure 6c);
- `ycsb_freshness`: vary the replication batch size on YCSB workload (Figure 6d).


For the experiments in Figure 7 and Figure 8, you can use the `test.py` script:

    python test.py

This script runs VeriTxn with a single RW node. Note that run_experiments.py also supports these experiments.

The used test cases in `test.py` are:

```
# vary the cache size (Figure 7)
run_cache_size_impact_for_different_methods_test() 

# compare with Litmus (Figure 8)
run_thread_exp()# vary thread count on YCSB workload (Figure 8a)
run_profiling() # enable profiling to obtain breakdown latency (Figure 8b)
run_theta_exp() # vary the skew factor on YCSB workload (Figure 8c)
run_tpc_exp()   # vary thread counts and the warehouse number on TPCC workload (Figure 8d)
```

### Q&A: How do the scripts work?

Our test scripts follow a systematic sequence to execute tests:

For the `run_experiments.py` script:

- *Configuration Adjustment*: The `run_experiments.py` script begins by reading configurations from `experiment.py` based on the supplied `<test_case_name>`. It then modifies the` common/config.h` file to match the required configurations for each experiment (lines 99-111).
- *Compilation*: The script re-compiles the code across all nodes.
- *Execution and Data Collection*: Subsequently, it runs the binary, performs the experiments, and collects the results into the designated result folder.

The `test.py` script operates on a similar principle: Within the `test_compile()` function (line 84), it alters the configuration in `common/config.h` and then compiles the code. The `test_run()` function (line 125) executes the binary to conduct the experiments.

For reference on default settings, check `experiment.py `(lines 247-297) and `test.py` (lines 34-39).


### Q&A: How can I introduce my own testcase?

If you're inclined to customize or introduce new test cases:

- Incorporate them into either `experiment.py` or `test.py` based on your specific requirements.
- Familiarize yourself with the current test cases for guidance. Their structure and comments should provide a clear and intuitive roadmap on how to add new cases.



Configurations
----------------------

You can modify various system settings within the `common/config.h` file. 
For clarity, we have highlighted key configurations used in our experiments below. 
All these configurations are consistent with those detailed in our paper.

For comprehensive definitions of each setting and the default configurations, please refer to the `README.txt.` 
**(Note: In our revision, we provided a file named `README`. However, due to display issues in the anonymous repository, this file wasn't shown. This might have caused you to overlook some clarifications regarding configurations. To address this, we have now renamed it to `README.txt`.)**


### General Settings

    WORKLOAD    : supported workloads include YCSB and TPCC
    NODE_CNT    : number of compute nodes modeled in the system
    THREAD_CNT  : number of worker threads running in the compute node
    CC_ALG		: concurrency control algorithm
    USE_SGX     : enable verification or not
    VERIFIED_CACHE_SIZ  : size of verified cache
    DATA_CACHE_SIZE     : size of data cache
    ENABLE_DATA_CACHE   : enable data cache or not
    VERI_TYPE           : verification method (DEFERRED_MEMORY, PAGE_VERI, MERKLE_TREE)
    INDEX_STRUCT      : index structure (IDX_BTREE, IDX_HASH)
    USE_LOG           : enable durable transaction redo log
    PROFILING         : enable profiling or not
    SYNC_VERSION_BATCH  : batch size for verified hash synchronization
    TAMPER_RECOVERY     : enable tamper recovery
    TAMPER_PERCENTAGE   : percentage of tampered data

The default values are:

    NODE_CNT : 4
    THREAD_CNT : 4
    CC_ALG   : OCC
    USE_SGX : 1
    VERIFIED_CACHE_SIZ  : 4 * 1024 * 1024 * 1024
    DATA_CACHE_SIZE     : 4 * 1024 * 1024 * 1024
    ENABLE_DATA_CACHE : true
    VERI_TYPE : PAGE_VERI
    INDEX_STRUCT  : IDX_BTREE
    USE_LOG : 1
    PROFILING   : false
    SYNC_VERSION_BATCH  : 16
    TAMPER_RECOVERY : 1
    TAMPER_PERCENTAGE : 0


### TPCC Specification

    NUM_WH		: number of warehouses being modeled
    FULL_TPCC   : if enabled, the system will utilize the complete set of TPCC functionalities
    TPCC_ACCESS_ALL : if enabled, the system will parse all attributes from the data row
    PERC_PAYMENT	: percentage of payment transactions. Note that this setting is only applicable when `TPCC_ACCESS_ALL` is set to `false`

The default values are:

    NUM_WH  : 16
    FULL_TPCC : true
    TPCC_ACCESS_ALL : false
    PERC_PAYMENT : 0.5

**!IMPORTANT**: `TPCC_ACCESS_ALL` Configuration Analysis

Although the `TPCC_ACCESS_ALL` configuration enables or disables certain attribute parsing within the TPCC benchmark, our experiments demonstrate that its impact on performance is minimal. 
Specifically, the overhead introduced by parsing this configuration is negligible.
We conducted experiments using the default settings to verify the overhead it introduces. 
The results are as follows:

| Throughput (Txn/s)     | 2 threads | 4 threads     | 8 threads |
| ----------- | ----------- | ----------- | ----------- |
| VeriTxn (TPCC_ACCESS_ALL = False)    | 13344.34651       | 20475.29013 | 18079.99355
| VeriTxn (TPCC_ACCESS_ALL = True)   |  13559.82545  | 20461.80959 | 17836.56319

*Note*: The throughput listed above match that presented in Figure 6b, further confirming the negligible performance difference introduced by the `TPCC_ACCESS_ALL` setting.


### YCSB Specification


    SYNTH_TABLE_SIZE : table size
    REQ_PER_QUERY	 : number of queries per transaction
    ZIPF_THETA	     : theta in zipfian distribution (rows accessed follow zipfian distribution)
    READ_PERC		 : read operation percentage
    WRITE_PERC	     : write operation percentage

The default values are:

    SYNTH_TABLE_SIZE  : 10 * 1000 * 1000
    REQ_PER_QUERY : 16
    ZIPF_THETA  : 0.5
    READ_PERC : 0.5
    WRITE_PERC : 0.5



TPCC Implementation and Fair Comparison
----------------------


Our TPCC implementation is derived from several transaction processing studies using the DBx1000 codebase:

- [Litmus](https://github.com/yuxiamit/LitmusDB/blob/main/dbx1000_logging/benchmarks/tpcc_txn.cpp): Litmus: Towards a Practical Database Management System with Verifiable ACID Properties and Transaction Correctness, SIGMOD 22
- [Cornus](https://github.com/CloudOLTP/Cornus/blob/cleanup/src/benchmarks/tpcc_store_procedure.cpp): Atomic commit for a cloud DBMS with storage disaggregation, VLDB 22
- [Ploris](https://github.com/chenhao-ye/polaris/blob/main/benchmarks/tpcc_txn.cpp): Enabling Transaction Priority in Optimistic Concurrency Control, SIGMOD 23 
- [Bamboo](https://github.com/ScarletGuo/Bamboo-Public/blob/dbx1000-bamboo/benchmarks/tpcc_txn.cpp): Releasing Locks As Early As You Can: Reducing Contention of Hotspots by Violating Two-Phase Locking, SIGMOD 21
- [Sundial](https://github.com/yxymit/Sundial/blob/master/benchmarks/tpcc_store_procedure.cpp): Harmonizing Concurrency Control and Caching
in a Distributed OLTP Database Management System, VLDB 18
- [Plor](https://github.com/chenyoumin1993/Plor/blob/master/benchmarks/tpcc_txn.cpp#L108): General Transactions with Predictable, Low Tail Latency, SIGMOD 22

We have thoroughly checked our implementation to ensure all the operational logics in the standard TPCC transaction are implemented. There are five types of transactions.
The NewOrder, Payment, and Delivery transactions are read-write transactions, while the Stock-Level and Order-Status transactions are read-only.
We use the standard TPCC by default, which consists of 45% of NewOrder, 43% of Payment, and 4% each of the remaining three types of transactions.
To compare with Litmus, we use a simple mix of 50% of NewOrder and 50% of Payment transactions following the Litmus's paper.

### Fair Comparison

**Note**: **We sought to provide a systematic and fair comparison solely on transaction processing performance. Consistent with the baseline systems, VeriTxn contains all the components for transaction processing in real-world databases, such as concurrency control, logging, data manipulation, etc.
That is, all these systems exclude the SQL layer (e.g., cursor) as it is orthogonal to transaction processing.**

- Both VeriTxn and Litmus use identical TPCC codes and settings.
- Both VeriTxn and LedgerDB conduct experiments using the standard TPCC.
Both LedgerDB and VeriTxn includes all the operational logics in the standard TPCC transactions.
<!-- This allows VeriTxn to enable the standard TPCC and employs the B$^{+}$-tree index. -->

### Cursor in TPCC

According to the standard TPCC (Page 108 of the TPC-C.pdf in [28]), using the cursor is not mandatory:

```
// The code presented here is for demonstration purposes only, 
// and is not meant to be an optimal implementation. 
...
EXEC SQL DECLARE c_byname CURSOR FOR
SELECT c_first, c_middle, c_id, c_street_1, c_street_2, c_city, c_state,
c_zip, c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since
FROM customer
WHERE c_w_id=:c_w_id AND c_d_id=:c_d_id AND c_last=:c_last
ORDER BY c_first;
EXEC SQL OPEN c_byname;
...
```

To the best of our knowledge, the cursor is typically used to traverse the result set returned from the database, and therefore, layered on top of transaction processing. 
For this reason, VeriTxn and the baseline systems do not support cursor. 


We would like to clarify that VeriTxn avoid the cursor but accommodate its essential logic (`trusted/system/tpcc_txn.cpp`, lines 145-155):

```
// This code is used to target a customer by the customer last name

// Step 1: obtaining a list of the related customer's primary 
// keys by the customer last name

uint64_t key = custNPKey(query->c_last, query->c_d_id, query->c_w_id);
item = index_read("CUSTOMER_LAST_IDX", key, wh_to_part(c_w_id));
if (item == nullptr) {
    return finish(Abort);
}
assert(item != NULL);

// Step 2: traversing this list to get the primary key of the customer

int cnt = 0;
itemid_t * it = item;
itemid_t * mid = item;
while (it != NULL) {
    cnt ++;
    it = it->next;
    if (cnt % 2 == 0)
        mid = mid->next;
}
r_cust = ((row_t *)mid->location);
```

This approach is consistent with that in Sysbench-TPCC (tpcc_run.lua, lines 316-337):


```
// Step 1: obtaining the related customer's primary 
// keys by the customer last name

--		SELECT c_id
--		FROM customer
--		WHERE c_w_id = :c_w_id 
--		AND c_d_id = :c_d_id 
--		AND c_last = :c_last
--		ORDER BY c_first;

	if tonumber(namecnt) >1 and namecnt % 2 == 1 then
		namecnt = namecnt + 1
	end

	rs = con:query(([[SELECT c_id
		 	    FROM customer%d
			   WHERE c_w_id = %d AND c_d_id= %d
                             AND c_last='%s' ORDER BY c_first]]
			):format(table_num, c_w_id, c_d_id, c_last ))

// Step 2: traversing the result set to get the primary key of the customer

	for i = 1,  (namecnt / 2 ) + 1 do
		row = rs:fetch_row()
		c_id = row[1]
	end
```

This approach is also consistent with that in [LedgerDB](https://github.com/nusdbsystem/LedgerDatabase/blob/main/distributed/exes/tpccClient.cc) (distributed/exes/tpccClient.cc line 209).


### Indexing

When `FULL_TPCC` is set to true, it is required to set `IDX_INDEX` to `IDX_BTREE`.
With these settings, we utilize the B+-tree for supporting range-based queries. 
This indexing implementation is also derived from the existing works we have referenced.

For example, the `CUSTOMER_LAST_INDEX` serves as a secondary index rooted in the standard TPCC. It indexes the `C_Last` field to a list containing the primary keys of related data items.

This list is structured as a linked list, with `itemid_t` (`common/helper.h`) as its pointer, which means its size is not fixed.


```
class itemid_t {
public:
	itemid_t() { next = nullptr; location = nullptr; valid = false; };
	itemid_t(Data_type type, void * loc) {
        this->type = type;
        this->location = loc;
        this->next = nullptr;
    };
	Data_type type;
	void * location;
	itemid_t * next; // next pointer
	bool valid;
	void init();
	bool operator==(const itemid_t &other) const;
	bool operator!=(const itemid_t &other) const;
	void operator=(const itemid_t &other);
};
```

Note that this secondary index is also used in Sysbench-TPCC (tpcc_common.lua, line 351):

```
con:query("CREATE INDEX idx_customer"..i.." ON customer"..i.." (c_w_id,c_d_id,c_last,c_first)")
```


Outputs
----------------------

If you run the experiments, the output file will be stored in `results/<timestamp>`. If run VeriTxn in manual, the results will be shown in the console. 

Here is an example of the result:

    [summary] total_txn_cnt=77662, total_abort_cnt=47, run_time=37.176587,
    time_wait=0.000000, time_ts_alloc=0.009211, time_man=0.000000, 
    time_index=10.488881, time_abort=0.282327, time_cleanup=0.000000, 
    time_cache=0.000000, latency=0.000479, deadlock_cnt=0, cycle_detect=0,
    dl_detect_time=0.000000, dl_wait_time=0.000000, time_query=0.007279,
    debug1=0.000000, debug2=0.000000, debug3=0.000000, debug4=0.000000,
    debug5=0.000000, avg_read_ts_dist=-nan, avg_ver=1.000129, 
    time_recover=0.000000


Here we list several most important metrics:
- `txn_cnt`: The total number of committed transactions. This number is close to but smaller than THREAD_CNT * MAX_TXN_PER_PART. When any worker thread commits MAX_TXN_PER_PART transactions, all the other worker threads will be terminated.

- `abort_cnt`: The total number of aborted transactions. A transaction may abort multiple times before committing. Therefore, abort_cnt can be greater than txn_cnt.

- `run_time`: The aggregated transaction execution time (in seconds) across all threads. run_time is approximately the program execution time * THREAD_CNT. Therefore, the `per-thread throughput` is `txn_cnt / run_time` and the `total throughput` is `txn_cnt / run_time * THREAD_CNT`.

- `latency`: Average latency of transactions.

<!-- 
If you run the experiments, the output file is stored in ```results/<timestamp>```. You can check the `tmp-OCC` file for the results, where each line is the x-axis, RW node throughput, total throughput, abort rate, average latency, and RO node latency.
 -->



Run Experiments Manually
----------------------

If you would like more control over the experimentation process, you can opt to run experiments manually with your desired configurations.

### Set IP Address

- Open the `ifconfig.txt` file.
- Each line in the file corresponds to the IP address of one node. Therefore, the NODE_CNT in `common/config.h` should match the total number of lines in this file.
- As an example, if you have set NODE_CNT=2, then the `ifconfig.txt` file should contain two IP addresses:


```
10.10.10.24
10.10.10.23
```

### Configure Settings

Please edit the `common/config.h` directly following the configurations defined in `README.txt`.

### Compile and Run

- After making the necessary modifications in the configuration, recompile the code.
- Once compiled, you can manually run VeriTxn on each node using the command:

```
./App 
```
