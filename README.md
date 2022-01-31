# LKH + SPH

## Disclaimer
!! NOTE: Work in progress !!
The project is provided as it is, as we used it during DIMACS competition. 
There are many rough corners and little aspects to polish, so be aware of that.
The authors are not responsible for broken machines, corrupted OS, thermonuclear 
war, the current economic crisis or anything that might be caused by using this 
program. 

``LKH + SPH`` or ``LKH-SP`` comes from the union of the improved version of Helsgaun's [LKH-3](https://github.com/c4v4/LKH3) algorithm with the CFT inspired [Set-Partitioning Heuristic](https://github.com/c4v4/sph).
With this algorithm, we took part to DIMASC 12th Implmentation Challenge on Vehicle Routing Problems:
- Team name: _Vavavuma!_
- Team memebers: _F. Cavaliere, M. Fischetti, K. Helsgaun_
- Algorithm name: _LKHSP_


## Project structure
For each variant, some of the originally dynamic variables and function pointers are now defined at compile time to help compiler optimizations.

- COMMON: Files shared between different VRP variants
  - LKH-3: Most of the LKH-3 source code + the SPH.hpp header-only library

- CVRP: Files specific for the CVRP variant    
- CVRPTW: Files specific for the CVRPTW variant
 
Both CVRP and CVRPTW contain:
- SRC: Specialized code
  - ExtractRoutes.cpp: Constraint checker
  - Forbidden.c: LKH-3 variant-specialized Forbidden function 
  - main.cpp: customized main function for the given variant
  - parameters.c: compile-time parameters (to help LTO)
  - Penalty.c: Variant Penalty implementation
  - ReadProblem.c: Instance parser

## Get
### Fresh clone:
Cloning for the first time the repo, the usual command "git clone" needs to be invoked with the "--recurse-submodules" flag. 
Therefore, the whole command should be (assuming a ssh access to github):

        $ git clone --recurse-submodules git@github.com:c4v4/LKH-SP.git

### Update of an already existing repository:
If, instead, you want to update an already existing local repository cloned with "git clone" without the previous flag, the following command should do the trick:

        $ git submodule update --init --recursive
## Run
The executable can be found in they respective directories (CVRP and CVRPTW).
The arguments are compatible with the respective DIMACS controllers.
        
        $ CVRP/cvrp <instance-file-path> <rounded> <time-limit> [<param-file>]
        $ CVRPTW/cvrptw <instance-file-path> <time-limit> [<param-file>]

Examples:
 
        $ CVRP/cvrp CVRPController/InstancesRounded/X-n524-k153.vrp 1 7200 CVRP/default_params.txt
        $ CVRPTW/cvrptw VRPTWController/Instances/Homberger/RC2_4_2.txt 3600 CVRPTW/default_params.txt
