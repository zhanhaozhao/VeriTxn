#python run_experiments.py -e -c vcloud tpcc_stress1
#sleep 10

python run_experiments.py -e -r -ns -c vcloud ycsb_writes
sleep 10
python run_experiments.py -e -r -c vcloud ycsb_writes
sleep 10

python run_experiments.py -e -r -ns -c vcloud ycsb_skew
sleep 10
python run_experiments.py -e -r -c vcloud ycsb_skew
sleep 10
# python run_experiments.py -e -c vcloud ycsb_scaling
# sleep 10
