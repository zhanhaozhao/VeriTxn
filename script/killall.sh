set -x

ssh -p 5000 10.0.0.5 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
ssh -p 5000 10.0.0.6 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
ssh -p 5000 10.0.0.7 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
ssh -p 5000 10.0.0.8 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
# ssh -p 5000 worker-035 "ps -aux | grep App | awk '{print \$2}' | xargs kill -9" 2>/dev/null 1>/dev/null
