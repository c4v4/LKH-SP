#!/bin/bash

#args=($(cat /proc/$PPID/cmdline | tr '\000' ' '))
# args[0]: Controller
# args[1]: Team name
# args[2]: Instance
# args[3]: Rounded|NotRounded|Explicit
# args[4]: CPU Score
# args[5]: Timelimit (not normalized)
# args[6]: BKS
# args[7]: Optimal
# args[8]: This script path
# args[9]: Custom parameter for tuning
#$(dirname $0)/cvrp $1 $2 $3 1 -1 ${args[9]} | tee DIMACS-$(basename -- ${1}).out
#$(dirname $0)/cvrp $1 $2 $3 1 $? ${args[9]} | tee DIMACS-$(basename -- ${1}).out

$(dirname $0)/cvrp $1 $2 $3 1 -1 1 
veh=$?
$(dirname $0)/cvrp $1 $2 $3 1 $veh 1 | tee DIMACS-$(basename -- ${1}).out
