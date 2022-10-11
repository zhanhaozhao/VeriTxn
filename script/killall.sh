set -x

ssh -p 5000 worker-033 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
ssh -p 5000 worker-034 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
ssh -p 5000 worker-035 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
