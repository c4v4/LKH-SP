PROJECT_HOME = ../../SRC
COMMON_HOME = ../../SRC/COMMON
ODIR = OBJ
CCIDIR = $(PROJECT_HOME)/LKH-3/INCLUDE
CXXIDIR = $(PROJECT_HOME)/SPH
CCFLAGS = -O3 -Wall -I$(CCIDIR) -g -fcommon -flto -fno-fat-lto-objects -fno-omit-frame-pointer
CXXFLAGS = $(CCFLAGS) -I$(CXXIDIR) -fno-exceptions
LKH_LIB = $(PROJECT_HOME)/libLKHSym.a

_CCDEPS = Delaunay.h GainType.h Genetic.h GeoConversion.h Hashing.h      \
        Heap.h LKH.h Segment.h Sequence.h BIT.h gpx.h

CCDEPS = $(patsubst %,$(CCIDIR)/%,$(_CCDEPS))

CXXDEPS = $(CCDEPS) $(CXXIDIR)/SPH.hpp

#_CCOBJ = cvrp.o 
#CCOBJ = $(patsubst %,$(ODIR)/%,$(_CCOBJ))

_CXXOBJ = main.opp cvrp.opp
CXXOBJ = $(patsubst %,$(ODIR)/%,$(_CXXOBJ))

.PHONY: 
	all clean

all:
	$(MAKE) cvrp

cvrp: $(CCOBJ) $(CXXOBJ) $(CXXDEPS) $(LKH_LIB)
	$(CXX) -o ../cvrp $(CCOBJ) $(CXXOBJ) $(CXXFLAGS) -L$(PROJECT_HOME) -lLKHSym -lm

#$(ODIR)/%.o: %.c $(CCDEPS) | $(ODIR)
#	$(CC) -c -o $@ $< $(CCFLAGS)

$(ODIR)/cvrp.opp: cvrp.cpp $(CXXDEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
	
$(ODIR)/main.opp: $(COMMON_HOME)/main.cpp $(CXXDEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(LKH_LIB):
	cd $(dir $@) && $(MAKE) $(notdir $@)

$(ODIR):
	mkdir -p $@

clean:
	/bin/rm -f $(ODIR)/*.o *~ ._* $(IDIR)/*~ $(IDIR)/._* 

