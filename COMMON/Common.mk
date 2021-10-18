# Makefile body for VRP variants Makefiles (e.g. CVRP/SRC/Makefile)
# Externally defined variables:
#  - PROJECT_HOME: where the project common files are placed
#  - LKH_LIB: the LKH-3 based static library to use (symetric or asymmetric)
#  - EXE: the name of the binary created (e.g. "cvrp" or "cvrptw")
#  - DEBUG: debug flags

CPLEX_IDIR = ${CPLEX_ROOT_DIR}/cplex/include/
CPLEX_LIB = ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic/
IDIR = $(PROJECT_HOME)/LKH-3/INCLUDE
CCFLAGS = -O3 -Wall -I$(IDIR) -g -flto -fno-fat-lto-objects $(DEBUG)
CXXFLAGS = $(CCFLAGS) -I$(CPLEX_IDIR) -fno-exceptions
ODIR = OBJ
LIBS = -L$(CPLEX_LIB) -L$(PROJECT_HOME) -lcplex -lLKHSym -lm -lpthread -ldl

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
	$(MAKE) -C $(dir $@) $(notdir $@) DEBUG=$(DEBUG)

$(ODIR):
	mkdir -p $@

clean:
	/bin/rm -f $(ODIR)/*.o* *~ ._* $(IDIR)/*~ $(IDIR)/._* $(LKH_LIB) ../$(EXE)