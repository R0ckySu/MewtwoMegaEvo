#!/bin/bash

export TIME_STAMP_CMD=$(date +'%Y%m%d%H%M%S')
export NUM_OF_GROUPS=5
export GROUP_INDEX=0
export EX_FOLDER="./sim_results/"
export CONFIG_PATH="./config_files.json"

let "TIME_STAMP=${TIME_STAMP_CMD}"

for ((i=0;i<${NUM_OF_GROUPS};i++));do
    let "GROUP_INDEX = i"

    touch qsub_div_${i}

    echo '#PBS -P oz09' >> qsub_div_${i}
    echo '#PBS -q normal' >> qsub_div_${i}
    echo '#PBS -l walltime=01:00:00,mem=160GB,ncpus=48' >> qsub_div_${i}
    echo '#PBS -l wd' >> qsub_div_${i}
    echo "./MewtwoMegaEvo -g ${NUM_OF_GROUPS} -i ${GROUP_INDEX} -t ${TIME_STAMP} -o ${EX_FOLDER} -c ${CONFIG_PATH}" >> qsub_div_${i}
    sleep 1
    #qsub qsub_div_${i}
done
