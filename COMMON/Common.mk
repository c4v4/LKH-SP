# Makefile body for VRP variants Makefiles (e.g. CVRP/SRC/Makefile)
# Externally defined variables:
#  - PROJECT_HOME: where the project common files are placed
#  - LKH_LIB: the LKH-3 based static library to use (symetric or asymmetric)
#  - EXE: the name of the binary created (e.g. "cvrp" or "cvrptw")
#  - DEBUG: debug flags

#CC = gcc
#CXX = g++
#AR = gcc-ar

CC = clang
CXX = clang++

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CPLEX_LIB_TYPE = linux
endif
ifeq ($(UNAME_S),Darwin)
	CPLEX_LIB_TYPE = osx
endif


CPLEX_IDIR = ${CPLEX_ROOT_DIR}/cplex/include/
CPLEX_LIB = ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_$(CPLEX_LIB_TYPE)/static_pic/
IDIR = $(PROJECT_HOME)/LKH-3/INCLUDE
#FLTO = -flto
CCFLAGS = -O3 -Wall -I$(IDIR)$(DEBUG) $(FLTO)
CXXFLAGS = $(CCFLAGS) -std=c++17 -I$(CPLEX_IDIR) -fno-exceptions
ODIR = OBJ
LIBS = -L$(CPLEX_LIB) -L$(PROJECT_HOME) $(LKH_LIB_FLAG) -lm -lpthread -ldl -lcplex

# C 

_DEPS = Delaunay.h GainType.h Genetic.h GeoConversion.h Hashing.h Heap.h      \
        LKH.h Segment.h Sequence.h BIT.h gpx.h extract_routes.hpp SPH.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS)) Makefile

_CCOBJ = Forbidden.o Penalty.o parameters.o 
CCOBJ = $(patsubst %,$(ODIR)/%,$(_CCOBJ))

$(ODIR)/%.o: %.c $(CCDEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CCFLAGS)

# C++

CXXOBJ = $(ODIR)/ExtractRoutes.opp

$(ODIR)/%.opp: %.cpp $(DEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(EXE): $(CCOBJ) $(CXXOBJ) $(DEPS) $(LKH_LIB)
	$(CXX) -o ../$(EXE) $(CCOBJ) $(CXXOBJ) $(CXXFLAGS) $(LIBS)
	

# GENERAL

.PHONY: 
	all clean

all:
	$(MAKE) $(EXE)

$(LKH_LIB):
	$(MAKE) -C $(dir $@) $(notdir $@) DEBUG="$(DEBUG)"

$(ODIR):
	mkdir -p $@

clean:
	/bin/rm -f $(ODIR)/*.o* *~ ._* $(IDIR)/*~ $(IDIR)/._* $(LKH_LIB) ../$(EXE)