# LKH + SPH

!! NOTE: Work in progress !!

LKH-3 base version [here](https://github.com/c4v4/LKH3).

## Project structure:
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
  - parameters.c: compile-time parameters (to help LTO)
  - Penalty.c: Variant Penalty implementation

## Run:
From the CVRP or CVRPTW folder, build with make and launch as:
		./cvrp <instance> [<sol-file>] [<seed>]
