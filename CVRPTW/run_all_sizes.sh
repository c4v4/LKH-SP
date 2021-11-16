#! /bin/sh

set +e
for inst in INSTANCES/Homberger_{2,4,6,8,10}00/$1_*_$2.*; do ./cvrptw $inst &> $(basename -- $inst).out; done;
set -e
