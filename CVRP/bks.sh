#!/bin/bash

for sol in BKS/*.sol; do
    base=$(basename -- $sol)
    base=${base%%.sol}
    cat default_params.txt >${base}-params.txt
    echo "INITIAL_SOL_FILE = ${sol}" >> ${base}-params.txt
    cat ${base}-params.txt
    echo""
    ./cvrp ../CVRPController/InstancesRounded/$base.vrp 1 100000000000 ${base}-params.txt >$base.out &
done
