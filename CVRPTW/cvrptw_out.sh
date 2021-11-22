#!/bin/sh

args=($(cat /proc/$PPID/cmdline | tr '\000' ' '))
# args[0]: Controller
# args[1]: Team name
# args[2]: Instance
# args[3]: CPU Score
# args[4]: Timelimit (not normalized)
# args[5]: BKS
# args[6]: Optimal
# args[7]: This script path
# args[8]: Custom parameter for tuning
if [ ${#args[@]} -lt 8 ]; then
    >&2 echo "Error, CPU score not found!"
    exit
fi

$(dirname $0)/cvrptw $1 $((2000 * ${args[4]} / ${args[3]})) | tee DIMACS-$(basename -- ${args[2]}).out
