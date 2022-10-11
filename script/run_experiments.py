#!/usr/bin/python

import os,sys,datetime,re
import shlex
import subprocess
from experiments import *
from helper import *
from run_config import *
import glob
import time

now = datetime.datetime.now()
strnow=now.strftime("%Y%m%d-%H%M%S")

os.chdir('..')

PATH=os.getcwd()

result_dir = PATH + "/results/" + strnow + '/'
perf_dir = result_dir + 'perf/'

cfgs = configs

execute = True
remote = False
cluster = None
skip = False
sgx = True

exps=[]
arg_cluster = False
merge_mode = False
perfTime = 60
fromtimelist=[]
totimelist=[]

if len(sys.argv) < 2:
     sys.exit("Usage: %s [-exec/-e/-noexec/-ne] [-c cluster] experiments\n \
            -exec/-e: compile and execute locally (default)\n \
            -noexec/-ne: compile first target only \
            -c: run remote on cluster; possible values: istc, vcloud\n \
            " % sys.argv[0])

for arg in sys.argv[1:]:
    if arg == "--help" or arg == "-h":
        sys.exit("Usage: %s [-exec/-e/-noexec/-ne] [-skip] [-c cluster] experiments\n \
                -exec/-e: compile and execute locally (default)\n \
                -noexec/-ne: compile first target only \
                -skip: skip any experiments already in results folder\n \
                -c: run remote on cluster; possible values: istc, vcloud\n \
                " % sys.argv[0])
    if arg == "--exec" or arg == "-e":
        execute = True
    elif arg == "--noexec" or arg == "-ne":
        execute = False
    elif arg == "--skip":
        skip = True
    elif arg == "-c":
        remote = True
        arg_cluster = True
    elif arg == '-m':
        merge_mode = True
    elif arg_cluster:
        cluster = arg
        arg_cluster = False
    elif arg == '-ns':
        sgx = False
    else:
        exps.append(arg)

for exp in exps:
    fmt,experiments = experiment_map[exp]()

    for e in experiments:
        cfgs = get_cfgs(fmt,e)
        if remote:
            cfgs["TPORT_TYPE"], cfgs["TPORT_PORT"] = "tcp", 6000
        if sgx:
            cfgs["USE_SGX"] = 1
        else:
            cfgs["USE_SGX"] = 0
        output_f = get_outfile_name(cfgs, fmt)
        output_dir = output_f + "/"
        output_f += strnow

        f = open("common/config.h", 'r')
        lines = f.readlines()
        f.close()
        with open("common/config.h", 'w') as f_cfg:
            for line in lines:
                found_cfg = False
                for c in cfgs:
                    found_cfg = re.search("#define " + c + "\t", line) or re.search("#define " + c + " ", line)
                    if found_cfg:
                        f_cfg.write("#define " + c + " " + str(cfgs[c]) + "\n")
                        break
                if not found_cfg:
                    f_cfg.write(line)
        if sgx:
            cmd = "cp Makefile.new Makefile"
        else:
            cmd = "cp Makefile.old Makefile"
        print cmd
        os.system(cmd)
        
        cmd = "make clean"
        print cmd
        os.system(cmd)

        cmd = "make -j10"
        print cmd
        os.system(cmd)

        if not execute:
            exit()

        if execute:
            cmd = "mkdir -p {}".format(perf_dir)
            print cmd
            os.system(cmd)
            cmd = "cp config.h {}{}.cfg".format(result_dir,output_f)
            print cmd
            os.system(cmd)

            if remote:
                if cluster == 'vcloud':
                    machines_ = vcloud_machines
                    uname = vcloud_uname
                    uname2 = username
                    cfg_fname = "vcloud_ifconfig.txt"
                else:
                    assert(False)

                machines = machines_[:(cfgs["NODE_CNT"])]
                with open("ifconfig.txt", 'w') as f_ifcfg:
                    for m in machines:
                        f_ifcfg.write(m + "\n")

                if cfgs["WORKLOAD"] == "TPCC":
                    files = ["App", "ifconfig.txt", "./untrusted/benchmarks/TPCC_short_schema.txt", "./benchmarks/TPCC_full_schema.txt"]
                elif cfgs["WORKLOAD"] == "YCSB":
                    files = ["App", "ifconfig.txt", "./untrusted/benchmarks/YCSB_schema.txt"]
                
                # !copy App to remote nodes
                for m, f in itertools.product(machines, files):
                    if cluster == 'vcloud':
                        # os.system('./scripts/kill.sh {}'.format(m))
                        cmd = 'scp -P {} {}/{} {}:/{}'.format(5000,PATH, f, m, uname)
                    print cmd
                    os.system(cmd)
                # !execute experiment
                print("Deploying: {}".format(output_f))

                os.chdir('./script')

                cmd = 'pwd'
                print cmd
                os.system(cmd)

                if cluster == 'vcloud':
                    cmd = './vcloud_deploy.sh \'{}\' /{}/ {} {} {}'.format(' '.join(machines), uname, cfgs["NODE_CNT"], perfTime, uname2)
                print cmd
                fromtimelist.append(str(int(time.time())) + "000")
                os.system(cmd)
                totimelist.append(str(int(time.time())) + "000")
                os.chdir('..')
                # todo: need to seperate node(r) and node(r/w)
                for m, n in zip(machines, range(len(machines))):
                    if cluster == 'vcloud':
                        cmd = 'scp -P {} {}:/{}/dbresults{}.out results/{}/{}_{}.out'.format(5000,m,uname,n,strnow,n,output_f)
                        print cmd
                        os.system(cmd)


    al = []
    for e in experiments:
        al.append(e[2])
    al = sorted(list(set(al)))

    sk = []
    for e in experiments:
        sk.append(e[-2])
    sk = sorted(list(set(sk)))

    wr = []
    for e in experiments:
        wr.append(e[-4])
    wr = sorted(list(set(wr)))

    cn = []
    for e in experiments:
        cn.append(e[1])
    cn = sorted(list(set(cn)))

    ld = []
    for e in experiments:
        ld.append(e[-3])
    ld = sorted(list(set(ld)))

    tpcc_ld = []
    for e in experiments:
        tpcc_ld.append(e[-1])
    tpcc_ld = sorted(list(set(tpcc_ld)))

    ccnt = []
    for e in experiments:
        ccnt.append(e[-1])
    ccnt = sorted(list(set(ccnt)))

    cmd = ''
    os.chdir('./script')
    if exp == 'ycsb_skew':
        cmd = './result.sh -a ycsb_skew -n {} -c {} -s {} -t {}'.format(str(cn[0]), ','.join([str(x) for x in al]), ','.join([str(x) for x in sk]), strnow)
    elif exp == 'ycsb_writes':
        cmd='./result.sh -a ycsb_writes -n {} -c {} --wr {} -t {}'.format(cn[0], ','.join([str(x) for x in al]), ','.join([str(x) for x in wr]), strnow)
    elif 'ycsb_scaling' in exp:
        cmd='./result.sh -a ycsb_scaling -n {} -c {} -t {} --ft {} --tt {}'.format(','.join([str(x) for x in cn]), ','.join([str(x) for x in al]), strnow, ','.join(fromtimelist), ','.join(totimelist))
    elif 'tpcc_scaling' in exp:
        cmd='./result.sh -a tpcc_scaling -n {} -c {} -t {}'.format(','.join([str(x) for x in cn]), ','.join([str(x) for x in al]), strnow)
    elif 'ycsb_stress' in exp:
        cmd='./result.sh -a ycsb_stress -n {} -c {} -s {} -l {} -t {}'.format(str(cn[0]), ','.join([str(x) for x in al]), str(sk[0]), ','.join([str(x) for x in ld]), strnow)
    elif 'tpcc_stress' in exp:
        cmd='./result.sh -a tpcc_stress -n {} -c {} -l {} -t {}'.format(str(cn[0]), ','.join([str(x) for x in al]), ','.join([str(x) for x in tpcc_ld]), strnow)
    elif 'tpcc_cstress' in exp:
        cmd='./result.sh -a tpcc_stress_ctx -n {} -c {} -l {} -C {} -t {} --ft {} --tt {}'.format(str(cn[0]), ','.join([str(x) for x in al]), ','.join([str(x) for x in ld]), ','.join([str(x) for x in ccnt]), strnow, ','.join(fromtimelist), ','.join(totimelist))
    print cmd
    os.system(cmd)
