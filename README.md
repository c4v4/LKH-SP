# LKH + SPH

!! NOTE: Work in progress !!

LKH-3 base version [here](https://github.com/c4v4/LKH3).

## Project structure
For each variant, some of the originally dynamic variables and function pointers are now defined at compile time to help compiler optimizations.

- COMMON: Files shared between different VRP variants
  - LKH-3: Most of the LKH-3 source code + the SPH.hpp header-only library

- CVRP: Files specific for the CVRP variant    
- CVRPTW: Files specific for the CVRPTW variant
 
Both CVRP and CVRPTW contain:
- INSTACES: Relative VRP variant instances
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
From the CVRP or CVRPTW folder, build with make and launch as:

        $ ./cvrp <instance> [<sol-file>] [<seed>]
