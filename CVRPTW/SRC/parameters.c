#include "Genetic.h"
#include "LKH.h"


#define MAX_TRIALS 50000
#define RUNS 10000

/**
 * To help linking-time optimizations (with -flto), some of the variables and functions that
 * can be defined at compile time are set const and the respective value is assigned.
 * From some *very* brief tests it seems to give something around 15-30% speed-up.
 * The search trajectory is different though, and I'm not really sure why.
 */

const enum Types ProblemType = CVRPTW;
const int Asymmetric = 1;
const int MoveType = 5;
const int MoveTypeSpecial = 1;
const int SubsequentMoveType = MoveType;
const int SubsequentMoveTypeSpecial = MoveTypeSpecial;
Node *BestMove(Node *t1, Node *t2, GainType *G0, GainType *Gain) { return BestSpecialOptMove(t1, t2, G0, Gain); }
Node *BestSubsequentMove(Node *t1, Node *t2, GainType *G0, GainType *Gain) { return BestSpecialOptMove(t1, t2, G0, Gain); }

void SetParameters() {
    AscentCandidates = 50;
    BackboneTrials = 0;
    Backtracking = 0;
    CandidateSetSymmetric = 0;
    CandidateSetType = POPMUSIC;
    Crossover = ERXT;
    DemandDimension = 1;
    Excess = -1;
    ExternalSalesmen = 0;
    ExtraCandidates = 0;
    ExtraCandidateSetSymmetric = 0;
    ExtraCandidateSetType = QUADRANT;
    Gain23Used = 0;
    GainCriterionUsed = 1;
    GridSize = 1000000.0;
    InitialPeriod = -1;
    InitialStepSize = 0;
    InitialTourAlgorithm = WALK;
    InitialTourFraction = 1.0;
    Kicks = 1;
    KickType = 4;
    MaxBreadth = INT_MAX;
    MaxCandidates = 5;
    MaxMatrixDimension = 20000;
    MaxPopulationSize = 10;
    MaxSwaps = 0;
    MaxTrials = MAX_TRIALS;
    MTSPDepot = 1;
    MTSPMinSize = 1;
    MTSPMaxSize = -1;
    MTSPObjective = -1;
    NonsequentialMoveType = -1;
    Optimum = MINUS_INFINITY;
    PatchingA = 1;
    PatchingC = 0;
    Precision = 1;
    POPMUSIC_InitialTour = 0;
    POPMUSIC_MaxNeighbors = 5;
    POPMUSIC_SampleSize = 10;
    POPMUSIC_Solutions = 50;
    POPMUSIC_Trials = 1;
    Recombination = IPT;
    RestrictedSearch = 1;
    Runs = RUNS;
    Salesmen = 1;
    Scale = -1;
    Seed = 1;
    StopAtOptimum = 1;
    Subgradient = 1;
    SubproblemSize = 0;
    SubsequentPatching = 1;
    TimeLimit = 7200;
    RunTimeLimit = 600;
    TraceLevel = 0;
}