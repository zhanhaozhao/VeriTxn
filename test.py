import os, sys, re, os.path
import platform
import string
import subprocess, datetime, time, signal

from collections import defaultdict, OrderedDict

def replace(filename, pattern, replacement):
	f = open(filename)
	s = f.read()
	f.close()
	s = re.sub(pattern,replacement,s)
	f = open(filename,'w')
	f.write(s)
	f.close()

jobs = OrderedDict()
dbms_cfg = ["config-std.h", "common/config.h"]
algs = ['NO_WAIT']
def insert_job(alg="NO_WAIT", workload="YCSB", thread_num=4, theta=0.6, bkt_fac = 1, read_perc=0.8, use_sgx=True):
	jobs[workload + '_th_num=' + str(thread_num) + '_theta=' + str(theta) + '_bkt_fac=' + str(bkt_fac) + "_readP=" + str(read_perc) + "SGX=" + str(use_sgx)] = {
		"WORKLOAD"			: workload,
		"CORE_CNT"			: thread_num,
		"CC_ALG"			: alg,
		"THREAD_CNT"		: thread_num,
		"ZIPF_THETA"		: theta,
		"BUCKET_FACTOR"		: bkt_fac,
		"READ_PERC"			: read_perc,
		"WRITE_PERC"		: 1-read_perc,
		"USE_SGX"			: use_sgx
	}


def test_compile(job):
	os.system("cp "+ dbms_cfg[0] +' ' + dbms_cfg[1])

	if job["USE_SGX"] == True:
		os.system("cp Makefile.sgx Makefile")
	else:
		os.system("cp Makefile.no-sgx Makefile")
		pattern = r"\#define\s*" + "USE_SGX" + r'.*'
		replacement = ""
		replace(dbms_cfg[1], pattern, replacement)

	for (param, value) in job.items():
		pattern = r"\#define\s*" + re.escape(param) + r'.*'
		replacement = "#define " + param + ' ' + str(value)
		replace(dbms_cfg[1], pattern, replacement)

	os.system("make clean> temp.out 2>&1")
	# print("clean finished!!!!")
	# time.sleep(1)
	# exit(0)
	ret = os.system("make -j8> temp.out")
	# print("make finished!!!!")
	if ret != 0:
		print ("ERROR in compiling job=")
		print (job)
		exit(0)
	print ("PASS Compile\t\talg=%s,\tworkload=%s" % (job['CC_ALG'], job['WORKLOAD']))

def test_run(job, fimeName, test = ''):
	print(job)
	app_flags = "" #m_txn->run_txn
	if test == 'read_write':
		app_flags = "-Ar -t1"
	if test == 'conflict':
		app_flags = "-Ac -t4"

	if job["USE_SGX"] == True:
		cmd = "./App %s" % (app_flags) + fimeName
	else:
		cmd = "./rundb %s" % (app_flags) + fimeName
	print(cmd)
	# start = datetime.datetime.now()
	cmd = cmd
	process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
	process.wait()
# timeout = 400 # in second
# while process.poll() is None:
# 	time.sleep(1)
# 	now = datetime.datetime.now()
# 	if (now - start).seconds > timeout:
# 		os.kill(process.pid, signal.SIGKILL)
# 		os.waitpid(-1, os.WNOHANG)
# 		print ("ERROR. Timeout cmd=%s" % cmd)
# 		exit(0)
# if "PASS" in process.stdout.read():
# 	if test != '':
# 		print ("PASS execution. \talg=%s,\tworkload=%s(%s)" % \
# 			(job["CC_ALG"], job["WORKLOAD"], test))
# 	else :
# 		print ("PASS execution. \talg=%s,\tworkload=%s" % \
# 			(job["CC_ALG"], job["WORKLOAD"]))
# 	return
# print ("FAILED execution. cmd = %s" % cmd)

testRound = 1

def run_all_test(jobs, filename) :
	filename = "./results/" + filename
	os.system("echo 'thread_cnt, txn_cnt, abort_cnt, execution_time, latency' > %s" % filename)
	for (jobname, job) in jobs.items():
		for ii in range(testRound):
			test_compile(job)
			test_run(job, ">> %s" % filename)

# run YCSB tests
def run_thread_exp():
	global jobs
	jobs = OrderedDict()
	# for th in [3, 6, 7]:
	for th in [1, 2, 3, 4, 5, 6, 7, 8]:
		for alg in algs:
			insert_job(alg, 'YCSB', thread_num=th, read_perc=0, theta=0.9)
	# print(jobs)
	run_all_test(jobs, "thread_ycsb.csv")

def run_tpc_exp():
	global jobs
	jobs = OrderedDict()
	# for th in [3, 6, 7]:
	for th in [ 5, 6, 7, 8]:
		for alg in algs:
			insert_job(alg, 'TPCC', thread_num=th, read_perc=0, theta=0.9)
	# print(jobs)
	run_all_test(jobs, "thread_tpc.csv")

def run_theta_exp():
	global jobs
	jobs = OrderedDict()
	for th in [0, 0.5, 0.8, 0.9, 0.95, 0.99]:
		for alg in algs:
			insert_job(alg, 'YCSB', theta=th, read_perc=0, thread_num=8)
	# print(jobs)
	run_all_test(jobs, "theta_ycsb.csv")

def run_bktsiz_exp():
	global jobs
	jobs = OrderedDict()
	for th in [1, 4, 16, 64, 256, 1024]:
		for alg in algs:
			insert_job(alg, 'YCSB', bkt_fac=th)
	# print(jobs)
	run_all_test(jobs, "bucket_siz.csv")


def run_rw_exp():
	global jobs
	jobs = OrderedDict()
	for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
		for alg in algs:
			insert_job(alg, 'YCSB', read_perc=th, thread_num=8, theta=0.9)
	# print(jobs)
	run_all_test(jobs, "rw_ycsb.csv")

# # run TPCC tests
# jobs = {}
# for alg in algs:
# 	insert_job(alg, 'TPCC')
# run_all_test(jobs)

def run_common_test():
	global jobs
	jobs = OrderedDict()
	# for th in [3, 6, 7]:
	insert_job("NO_WAIT", 'YCSB', thread_num=1, read_perc=0.5, theta=0.6, use_sgx=True)
#	insert_job("NO_WAIT", 'YCSB', thread_num=4, read_perc=0.5, theta=0.6, use_sgx=True)
	# print(jobs)
	run_all_test(jobs, "comparison.csv")

run_common_test()
# run_rw_exp()
# run_bktsiz_exp()
#run_thread_exp()
#run_tpc_exp()
# run_theta_exp()

# os.system('cp config-std.h ./common/config.h')
os.system("cp Makefile.sgx Makefile")
# os.system('make clean > temp.out 2>&1')
# os.system('rm temp.out')
