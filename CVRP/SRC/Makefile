CPLEX_IDIR = ${CPLEX_ROOT_DIR}/cplex/include/
CPLEX_LIB = ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic/
PROJECT_HOME = ../../COMMON
LKHSP_HOME = $(PROJECT_HOME)/SRC
CCIDIR = $(PROJECT_HOME)/LKH-3/INCLUDE
CXXIDIR = $(LKHSP_HOME)/INCLUDE
CCFLAGS = -O3 -Wall -I$(CCIDIR) -g -flto -fno-fat-lto-objects -fno-omit-frame-pointer
CXXFLAGS = $(CCFLAGS) -I$(CXXIDIR) -I$(CPLEX_IDIR) -fno-exceptions
LKH_LIB = $(PROJECT_HOME)/libLKHSym.a
ODIR = OBJ

_CCDEPS = Delaunay.h GainType.h Genetic.h GeoConversion.h Hashing.h      \
        Heap.h LKH.h Segment.h Sequence.h BIT.h gpx.h
CCDEPS = $(patsubst %,$(CCIDIR)/%,$(_CCDEPS))

_CXXDEPS = SPH.hpp extract_routes.hpp 
CXXDEPS = $(CCDEPS) $(patsubst %,$(CXXIDIR)/%,$(_CXXDEPS))

_CCOBJ = parameters.o 
CCOBJ = $(patsubst %,$(ODIR)/%,$(_CCOBJ))

_CXXOBJ = main.opp cvrp.opp
CXXOBJ = $(patsubst %,$(ODIR)/%,$(_CXXOBJ))

.PHONY: 
	all clean

all:
	$(MAKE) cvrp

cvrp: $(CCOBJ) $(CXXOBJ) $(CXXDEPS) $(LKH_LIB)
	$(CXX) -o ../cvrp $(CCOBJ) $(CXXOBJ) $(CXXFLAGS) -L$(CPLEX_LIB) -lcplex -L$(PROJECT_HOME) -lLKHSym -lm -lpthread -ldl

$(ODIR)/%.o: %.c $(CCDEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CCFLAGS)

$(ODIR)/cvrp.opp: cvrp.cpp $(CXXDEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
	
$(ODIR)/main.opp: $(LKHSP_HOME)/main.cpp $(CXXDEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(LKH_LIB):
	cd $(dir $@) && $(MAKE) $(notdir $@)

$(ODIR):
	mkdir -p $@

clean:
	/bin/rm -f $(ODIR)/*.o* *~ ._* $(IDIR)/*~ $(IDIR)/._* $(LKH_LIB)

