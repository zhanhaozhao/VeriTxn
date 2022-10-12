# set -x

PHASE=5

while [[ $# -gt 0 ]]
do
    case $1 in
        -a)
            TEST_TYPE=$2
            shift
            shift
            ;;
        -c)
            CC=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -C)
            CT=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -T)
            TCNT=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        --tt)
            TT=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        --ft)
            FT=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -n)
            NUMBEROFNODE=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -p)
            PHASE=$2
            shift
            shift
            ;;
        -s)
            SKEW=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -t)
            RESULT_PATH=../results/$2
            shift
            shift
            ;;
        --wr)
            WR=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        -l)
            LOAD=($(echo $2 | tr ',' ' '))
            shift
            shift
            ;;
        *)
            shift
            ;;
    esac
done

ArgsType() {
    if [[ "${TEST_TYPE}" == 'ycsb_skew' ]]
    then
        args=("${SKEW[@]}")
    elif [[ "${TEST_TYPE}" == 'ycsb_scaling' ]]
    then
        args=("${NUMBEROFNODE[@]}")
    elif [[ "${TEST_TYPE}" == 'ycsb_writes' ]]
    then
        args=("${WR[@]}")
    elif [[ "${TEST_TYPE}" == 'tpcc_scaling' ]]
    then
        args=("${NUMBEROFNODE[@]}")
    elif [[ "${TEST_TYPE}" == 'ycsb_stress' ]]
    then
        args=("${LOAD[@]}")
    elif [[ "${TEST_TYPE}" == 'tpcc_stress' ]]
    then
        args=("${LOAD[@]}")
    elif [[ "${TEST_TYPE}" == 'tpcc_stress_ctx' ]]
    then
        args=("${LOAD[@]}")
    elif [[ "${TEST_TYPE}" == 'tpcc_thread' ]]
    then
        args=("${TCNT[@]}")
    fi   
}

FileName() {
    if [[ "${TEST_TYPE}" == 'ycsb_skew' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep ${cc} | grep _SKEW-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'ycsb_scaling' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep [0-9]_${cc}_ | grep _N-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'ycsb_writes' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep ${cc} | grep _WRITE_PERC-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'tpcc_scaling' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep [0-9]_${cc}_ | grep _N-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'ycsb_stress' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep ${cc}_TIF-${arg}_ | grep _SKEW-${SKEW[0]}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'tpcc_stress' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep ${cc}_TIF-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'tpcc_stress_ctx' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep [0-9]_${cc}_ | grep _CT-${CT}_TIF-${arg}_ | grep ^${i}_)
    elif [[ "${TEST_TYPE}" == 'tpcc_thread' ]]
    then
        f=$(ls ${RESULT_PATH} | grep -v .cfg | grep [0-9]_${cc}_ | grep _T-${arg}_ | grep ^${i}_)
    fi
}

TmpFileNum() {
    if [[ "${TEST_TYPE}" == 'ycsb_scaling' ]]
    then
        TMPN=${arg}
    elif [[ "${TEST_TYPE}" == 'tpcc_scaling' ]]
    then
        TMPN=${arg}
    else
        TMPN=${NUMBEROFNODE[0]}
    fi
}

# 通用的结果解析部分
LATFILE=lat
LTFILE=lt
rm -rf ${LATFILE} ${LTFILE}
touch ${LATFILE} ${LTFILE}
# echo "根据测试，确定第一个循环体类型"
ArgsType
#根据测试，确定第一个循环体类型
for cc in ${CC[@]}
do
    LS=''
    echo -n ${cc}" " >> ${LATFILE}
    TMPFILE=tmp-${cc}
    rm -rf ${TMPFILE}
    touch ${TMPFILE}
    IDLEFILE=idle-${cc}
    rm -rf ${IDLEFILE}
    touch ${IDLEFILE}
    CCLATFILE=lat-${cc}
    rm -rf ${CCLATFILE}
    touch ${CCLATFILE}
    DIS_FILE=dis-${cc}
    rm -rf ${DIS_FILE}
    touch ${DIS_FILE}
    touch ${DIS_FILE}
    # echo "根据测试，确定第2个循环体类型"
    #根据测试，确定第2个循环体类型
    ArgsType
    #根据测试，确定第2个循环体类型
    for arg in ${args[@]}
    do
        echo -n ${arg}" " >> ${TMPFILE}
        echo -n ${arg}" " >> ${CCLATFILE}
        echo -n ${arg}" " >> ${IDLEFILE}
        echo -n ${arg}" " >> ${DIS_FILE}
        AS=''
        # echo "根据测试，确定TMPN"
        #根据测试，确定TMPN
        TmpFileNum
        #根据测试，确定TMPN
        let TMPN--
        for i in $(seq 0 $TMPN)
        do
            # echo "根据测试，确定文件名"
            #根据测试，确定文件名
            FileName
            #根据测试，确定文件名            
            AS=${AS}$(readlink -f ${RESULT_PATH}/$f)" "
            LS=${LS}$(readlink -f ${RESULT_PATH}/$f)" "
        done
        tmpresult=$(python parse_results.py $AS)
        echo ${tmpresult} >> ${TMPFILE}
        # dis_tmpresult=$(python pl/parse_latency_dis.py $AS)
        # echo ${dis_tmpresult} >> ${DIS_FILE}
        # python parse_latency.py $AS >> ${CCLATFILE}
        # python parse_cpu_idle.py $AS >> ${IDLEFILE}
        tput=$(echo ${tmpresult} | awk '{print $1}')
        ar=$(echo ${tmpresult} | awk '{print $2}')
        dr=$(echo ${tmpresult} | awk '{print $3}')

    done
    # python parse_trans_latency.py $LS >> ${LTFILE}
    mv ${DIS_FILE} ${RESULT_PATH}/
    mv ${TMPFILE} ${RESULT_PATH}/
    mv ${CCLATFILE} ${RESULT_PATH}/
    mv ${IDLEFILE} ${RESULT_PATH}/
    cp ${LTFILE} ${RESULT_PATH}/
done
echo >> ${LATFILE}
echo "abort_time txn_manager_time txn_validate_time txn_cleanup_time txn_total_process_time" >> ${LATFILE}
awk -F' ' '{for(i=1;i<=NF;i=i+1){a[NR,i]=$i}}END{for(j=1;j<=NF;j++){str=a[1,j];for(i=2;i<=NR;i++){str=str " " a[i,j]}print str}}' ${LTFILE} >> ${LATFILE}
