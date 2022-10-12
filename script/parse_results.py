import re, sys

summary = {}


def get_summary(sfile):
    with open(sfile, 'r') as f:
        for line in f:
            if 'summary' in line:
                results = re.split(', ', line.rstrip('\n')[10:])
                for r in results:
                    (name, val) = re.split('=', r)
                    val = float(val)
                    if name not in summary.keys():
                        summary[name] = [val]
                    else:
                        summary[name].append(val)


for arg in sys.argv[1:]:
    get_summary(arg)
names = summary.keys()

a, b, c = 0, 0, 0
d, e = 0, 0
f, g = 0, 0
if 'total_txn_cnt' in summary :
    # a = sum(summary['txn_cnt']) / sum(summary['run_time'])
    a = sum(summary['total_txn_cnt'])
    d = summary['total_txn_cnt'][0]
if 'total_abort_cnt' in summary and 'total_txn_cnt' in summary and summary['total_txn_cnt'][0] + summary['total_abort_cnt'][0] != 0:
    b = summary['total_abort_cnt'][0] / (summary['total_txn_cnt'][0] + summary['total_abort_cnt'][0])
if 'run_time' in summary:
    c = sum(summary['run_time'])
    e = summary['run_time'][0]
if 'latency' in summary:
    f = summary['latency'][0]
    len = len(summary['latency'])
    if len>1:
        g = summary['latency'][1]
print d/e, a/c, b, f, g
