#!/bin/bash

LOCAL_UNAME="$5"
USERNAME="$5"
HOSTS="$1"
PATHE="$2"
NODE_CNT="$3"
count=0

for HOSTNAME in ${HOSTS}; do
    #SCRIPT="env SCHEMA_PATH=\"$2\" timeout -k 10m 10m gdb -batch -ex \"run\" -ex \"bt\" --args ./rundb -nid${count} >> results.out 2>&1 | grep -v ^\"No stack.\"$"
    if [ $count -ge 1 ]; then
        # SCRIPT="source /etc/profile;env SCHEMA_PATH=\"$2\" timeout -k 15m 15m ${PATHE}App -nid${count} -r100 -w0 > ${PATHE}dbresults.out 2>&1"
        SCRIPT="cd ${PATHE}; ./App -nid${count} -r100 -w0 > ${PATHE}dbresults${count}.out 2>&1"
        echo "${HOSTNAME}: App r ${count}"
        ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}" &
        echo $SCRIPT
    else
        SCRIPT="cd ${PATHE}; ./Store > ${PATHE}storageresults${count}.out 2>&1"
        echo "${HOSTNAME}: Store ${count}"
        ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}" &
        echo $SCRIPT
        sleep 20
        SCRIPT="cd ${PATHE}; ./App -nid${count} > ${PATHE}dbresults${count}.out 2>&1"
        echo "${HOSTNAME}: App r/w ${count}"
        ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}" &
        echo $SCRIPT
    fi
    count=`expr $count + 1`
done

sleep 2

while [ $count -gt 0 ]
do
    # echo $count
    if [ $count -eq 1 ]; then
        isrunning=$(ssh -p 5000 $USERNAME@$HOSTNAME "ps -aux | grep App | grep -v grep | wc -l")
        # echo $isrunning
        if [ $isrunning == 0 ]; then
            echo "not running"
            ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "ps -aux | grep Store | awk '{print \$2}' | xargs kill -9" 
        else
            # echo "still running"
            sleep 2
            continue
        fi
    fi

    wait $pids
    count=`expr $count - 1`
done
