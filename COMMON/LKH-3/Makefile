CUSTOM=-DCAVA_CUSTOM # Uncomment to activate all changes
#CUSTOM_PENALTY=-DCAVA_PENALTY # Uncomment to activate CVRP and CVRPTW penalty optimization
#CUSTOM_CACHE=-DCAVA_CACHE # Uncomment to activate the cost function cache optimization
#CUSTOM_FLIP=-DCAVA_FLIP # Uncomment to activate the one-level tree Flip function optimization (needs TREE_TYPE = ONE_LEVEL_TREE)

TREE_TYPE = TWO_LEVEL_TREE
#TREE_TYPE = THREE_LEVEL_TREE
#TREE_TYPE = ONE_LEVEL_TREE

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
#CC = gcc
#CXX = g++
#AR = gcc-ar rcs
CC = clang
CXX = clang++
AR = ar rcs
endif

ifeq ($(UNAME_S),Darwin)
	AR = libtool -c -static -o
endif



IDIR = INCLUDE
CPLEX_IDIR = ${CPLEX_ROOT_DIR}/cplex/include/
ODIR_SYM = OBJ_SYM
ODIR_ASYM = OBJ_ASYM
OPTIONS=$(CUSTOM) $(CUSTOM_PENALTY) $(CUSTOM_CACHE) $(CUSTOM_FLIP)
#DEBUG = -g -O0 -fno-omit-frame-pointer -DNDEBUG
FLTO = -flto
CFLAGS = -O3 -Wall -I$(IDIR) -D$(TREE_TYPE) $(FLTO) $(OPTIONS) $(DEBUG)
CXXFLAGS = $(CFLAGS) -std=c++17 -I$(CPLEX_IDIR) -fno-exceptions

_DEPS = Delaunay.h GainType.h Genetic.h GeoConversion.h Hashing.h Heap.h   \
       LKH.h Segment.h Sequence.h BIT.h gpx.h extract_routes.hpp SPH.hpp   \
	   SimulatedAnnealing.h

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS)) Makefile

_OBJ = Activate.o AddCandidate.o AddExtraCandidates.o                  \
       AddTourCandidates.o AdjustCandidateSet.o AdjustClusters.o       \
       AllocateStructures.o Ascent.o                                   \
       Best2OptMove.o Best3OptMove.o Best4OptMove.o Best5OptMove.o     \
       BestKOptMove.o BestSpecialOptMove.o                             \
       Between.o Between_SL.o Between_SSL.o                            \
       BridgeGain.o BuildKDTree.o C.o CandidateReport.o                \
       ChooseInitialTour.o Connect.o CreateCandidateSet.o              \
       CreateDelaunayCandidateSet.o CreateNNCandidateSet.o             \
       Create_POPMUSIC_CandidateSet.o CreateQuadrantCandidateSet.o     \
	   CTSP_InitialTour.o CVRP_InitialTour.o Delaunay.o                \
       Distance.o Distance_MTSP.o Distance_SOP.o Distance_SPECIAL.o    \
       eprintf.o ERXT.o Excludable.o Exclude.o FindTour.o              \
       FixedOrCommonCandidates.o Flip.o Flip_SL.o Flip_SSL.o           \
       FreeStructures.o                                    			   \
       fscanint.o Gain23.o GenerateCandidates.o Genetic.o              \
       GeoConversion.o GetTime.o GreedyTour.o Hashing.o Heap.o         \
       Improvement.o IsBackboneCandidate.o IsCandidate.o               \
       IsCommonEdge.o IsPossibleCandidate.o KSwapKick.o LinKernighan.o \
	   LKH.o 														   \
       Make2OptMove.o Make3OptMove.o Make4OptMove.o Make5OptMove.o     \
       MakeKOptMove.o MergeTourWithBestTour.o MergeWithTourIPT.o       \
       Minimum1TreeCost.o MinimumSpanningTree.o                        \
       MTSP2TSP.o MTSP_InitialTour.o MTSP_Report.o                     \
       MTSP_WriteSolution.o                                            \
       NormalizeNodeList.o NormalizeSegmentList.o                      \
       OrderCandidateSet.o PatchCycles.o                               \
       PDPTW_Reduce.o printff.o PrintParameters.o                      \
       Random.o ReadProblem.o ReadLine.o RecordBestTour.o 			   \
       RecordBetterTour.o RemoveFirstActive.o                          \
       ResetCandidateSet.o RestoreTour.o                               \
       SegmentSize.o Sequence.o SetInitialTour.o SFCTour.o  		   \
       SINTEF_WriteSolution.o SOP_RepairTour.o STTSP2TSP.o             \
	   SimulatedAnnealing.o											   \
       SOP_InitialTour.o SOP_Report.o StatusReport.o                   \
       Statistics.o StoreTour.o SymmetrizeCandidateSet.o               \
       TrimCandidateSet.o TSPDL_InitialTour.o TSPTW_MakespanCost.o     \
	   TSPTW_Reduce.o ValidateTour.o 								   \
	   VRPB_Reduce.o BIT.o        									   \
       WriteCandidates.o WritePenalties.o WriteTour.o WriteSolFile.o   \
       MergeWithTourGPX2.o gpx.o									   \
	   main.opp ReadInitialTourSol.opp

OBJ_SYM = $(patsubst %,$(ODIR_SYM)/%,$(_OBJ))
OBJ_ASYM = $(patsubst %,$(ODIR_ASYM)/%,$(_OBJ))

.PHONY: 
	all clean

all:
	$(MAKE) libLKHSym.a libLKHAsym.a

libLKHSym.a: $(OBJ_SYM) $(DEPS) 
	$(AR) ../$@ $(OBJ_SYM)
	
libLKHAsym.a: $(OBJ_ASYM) $(DEPS)
	$(AR) ../$@ $(OBJ_ASYM)

$(ODIR_SYM)/%.opp $(ODIR_ASYM)/%.opp: %.cpp $(DEPS) | $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(ODIR_SYM)/%.o $(ODIR_ASYM)/%.o: %.c $(DEPS) | $(ODIR_SYM) $(ODIR_ASYM)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR_SYM) $(ODIR_ASYM):
	mkdir -p $@

clean:
	/bin/rm -f $(ODIR_SYM)/*.o* $(ODIR_ASYM)/*.o* ../libLKH*.a *~ ._* $(IDIR)/*~ $(IDIR)/._* 



