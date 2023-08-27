#!/usr/bin/python

#Configuration file for run_experiments.py

# vcloud cluster setup

username = "root"
port = "5000"
vcloud_uname = 'opt/intel/sgxsdk/DBx1000'
identity = ".ssh/id_rsa_vcloud"

vcloud_machines = [
"10.10.10.58", # 070
"10.10.10.45", # 071
"10.10.10.62", # 072
"10.10.10.44", # 073
"10.10.10.48", # 074
"10.10.10.50", # 075
"10.10.10.12", # 076
"10.10.10.36", # 077
]




# =========
# ISTC cluster setup
istc_uname = 'rhardin'
# ISTC Machines ranked by clock skew
istc_machines=["istc1",]
