import os, sys, re, os.path
import platform
import subprocess, datetime, time, signal

def replace(filename, pattern, replacement):
	f = open(filename)
	s = f.read()
	f.close()
	s = re.sub(pattern,replacement,s)
	f = open(filename,'w')
	f.write(s)
	f.close()

jobs = {}
dbms_cfg = ["config-std.h", "common/config.h"]
algs = ['NO_WAIT']
def insert_job(alg, workload, thread_num=16, theta=0.6, bkt_fac = 1, read_perc=0.8):
	jobs[alg + '_' + workload + '_' + str(thread_num) + '_' + str(theta) + '_' + str(bkt_fac)] = {
		"WORKLOAD"			: workload,
		"CORE_CNT"			: 8,
		"CC_ALG"			: alg,
		"THREAD_CNT"		: thread_num,
		"ZIPF_THETA"		: theta,
		"BUCKET_FACTOR"		: bkt_fac,
		"READ_PERC"			: read_perc,
		"WRITE_PERC"		: 1-read_perc,
	}


def test_compile(job):
	os.system("cp "+ dbms_cfg[0] +' ' + dbms_cfg[1])
	for (param, value) in job.items():
		pattern = r"\#define\s*" + re.escape(param) + r'.*'
		replacement = "#define " + param + ' ' + str(value)
		replace(dbms_cfg[1], pattern, replacement)
	os.system("make clean > temp.out 2>&1")
	ret = os.system("make -j8 > temp.out 2>&1")
	if ret != 0:
		print ("ERROR in compiling job=")
		print (job)
		exit(0)
	print ("PASS Compile\t\talg=%s,\tworkload=%s" % (job['CC_ALG'], job['WORKLOAD']))

def test_run(job, fimeName, test = ''):
	print(job)
	app_flags = ""
	if test == 'read_write':
		app_flags = "-Ar -t1"
	if test == 'conflict':
		app_flags = "-Ac -t4"
	
	#os.system("./rundb %s > temp.out 2>&1" % app_flags)
	#cmd = "./rundb %s > temp.out 2>&1" % app_flags
	cmd = "./rundb %s" % (app_flags)
	start = datetime.datetime.now()
	cmd = cmd +  fimeName
	process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
	timeout = 400 # in second
	while process.poll() is None:
		time.sleep(1)
		now = datetime.datetime.now()
		if (now - start).seconds > timeout:
			os.kill(process.pid, signal.SIGKILL)
			os.waitpid(-1, os.WNOHANG)
			print ("ERROR. Timeout cmd=%s" % cmd)
			exit(0)
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

def run_all_test(jobs) :
	for (jobname, job) in jobs.items():
		for ii in range(testRound):
			test_compile(job)
			if ii == 0:
				test_run(job, ">./results/%s.log" % jobname)
			else:
				test_run(job, ">>./results/%s.log" % jobname)

# run YCSB tests
def run_thread_exp():
	global jobs
	jobs = {}
	for th in [1, 2, 4, 8, 16]:
		for alg in algs:
			insert_job(alg, 'YCSB', thread_num=th)
	print(jobs)
	run_all_test(jobs)

def run_theta_exp():
	global jobs
	jobs = {}
	for th in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
		for alg in algs:
			insert_job(alg, 'YCSB', theta=th)
	print(jobs)
	run_all_test(jobs)

def run_bktsiz_exp():
	global jobs
	jobs = {}
	for th in [1, 2, 4, 8, 16]:
		for alg in algs:
			insert_job(alg, 'YCSB', bkt_fac=th)
	print(jobs)
	run_all_test(jobs)

# # run TPCC tests
# jobs = {}
# for alg in algs:
# 	insert_job(alg, 'TPCC')
# run_all_test(jobs)

# run_bktsiz_exp()
#run_thread_exp()
run_theta_exp()

os.system('cp config-std.h ./common/config.h')
os.system('make clean > temp.out 2>&1')
os.system('rm temp.out')
