#!/bin/bash
set -x
LOCAL_UNAME="$5"
USERNAME="$5"
HOSTS="$1"
PATHE="$2"
NODE_CNT="$3"
count=0
USE_SGX="$6"

for HOSTNAME in ${HOSTS}; do
    if [ $count -eq 0 ]; then
        SCRIPT="cd ${PATHE}script; ./storage_compile.sh"
        echo "${HOSTNAME}: make Storage"
        ssh -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}"
    fi

    echo "USE SGX: $USE_SGX"
    if [ "$USE_SGX" != "True" ]; then
        SCRIPT="cd ${PATHE}; make no-sgx; make clean; make -j10"
    else
        SCRIPT="cd ${PATHE}; sed -i '251c #define USE_SGX 1' common/config.h; make sgx-release; make clean; make -j10"
    fi

    echo "${HOSTNAME}: make App"
    ssh -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}"
    # BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}" &
    echo $SCRIPT
    count=`expr $count + 1`
done
