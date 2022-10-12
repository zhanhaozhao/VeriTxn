#!/bin/bash
set -x
LOCAL_UNAME="$5"
USERNAME="$5"
HOSTS="$1"
PATHE="$2"
NODE_CNT="$3"
count=0

for HOSTNAME in ${HOSTS}; do
    SCRIPT="cd ${PATHE}; make clean"
    # echo "${HOSTNAME}: make clean"
    ssh -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}"
    # BatchMode=yes -o StrictHostKeyChecking=no -p 5000 -l ${USERNAME} ${USERNAME}@${HOSTNAME} "${SCRIPT}" &
    echo $SCRIPT
    # count=`expr $count + 1`
done
