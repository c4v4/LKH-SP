
extern "C" {
#include "BIT.h"
#include "Genetic.h"
#include "LKH.h"
}

#include "SPH.hpp"

void *sph_ptr;

int main(int argc, char *argv[]) {
    GainType Cost, OldOptimum;
    double Time, LastTime;
    Node *N;
    int i;

    /* Read the specification of the problem */
    if (argc >= 2) ParameterFileName = argv[1];
    ReadParameters();
    StartTime = LastTime = GetTime();
    MaxMatrixDimension = 20000;
    MergeWithTour = Recombination == IPT ? MergeWithTourIPT : MergeWithTourGPX2;
    ReadProblem();
    AllocateStructures();
    CreateCandidateSet();
    InitializeStatistics();

    Norm = 9999;
    BestCost = PLUS_INFINITY;
    BestPenalty = CurrentPenalty = PLUS_INFINITY;

    SPHeuristic sph(Dimension - 1);
    sph_ptr = &sph;

    /* Find a specified number (Runs) of local optima */

    for (Run = 1; Run <= Runs; Run++) {
        LastTime = GetTime();
        if (LastTime - StartTime >= TimeLimit) {
            if (TraceLevel >= 1) printff("*** Time limit exceeded ***\n");
            break;
        }
        Cost = FindTour(); /* using the Lin-Kernighan heuristic */
        if (MaxPopulationSize > 1 && !TSPTW_Makespan) {
            /* Genetic algorithm */
            int i;
            for (i = 0; i < PopulationSize; i++) {
                GainType OldPenalty = CurrentPenalty;
                GainType OldCost = Cost;
                Cost = MergeTourWithIndividual(i);
                if (TraceLevel >= 1 && (CurrentPenalty < OldPenalty || (CurrentPenalty == OldPenalty && Cost < OldCost))) {
                    if (CurrentPenalty) printff("  Merged with %d: Cost = " GainFormat, i + 1, Cost);
                    else
                        printff("  Merged with %d: Cost = " GainFormat "_" GainFormat, i + 1, CurrentPenalty, Cost);
                    if (Optimum != MINUS_INFINITY && Optimum != 0) { printff(", Gap = %0.4f%%", 100.0 * (Cost - Optimum) / Optimum); }
                    printff("\n");
                }
            }
            if (!HasFitness(CurrentPenalty, Cost)) {
                if (PopulationSize < MaxPopulationSize) {
                    AddToPopulation(CurrentPenalty, Cost);
                    if (TraceLevel >= 1) PrintPopulation();
                } else if (SmallerFitness(CurrentPenalty, Cost, PopulationSize - 1)) {
                    i = ReplacementIndividual(CurrentPenalty, Cost);
                    ReplaceIndividualWithTour(i, CurrentPenalty, Cost);
                    if (TraceLevel >= 1) PrintPopulation();
                }
            }
        } else if (Run > 1 && !TSPTW_Makespan)
            Cost = MergeTourWithBestTour();
        if (CurrentPenalty < BestPenalty || (CurrentPenalty == BestPenalty && Cost < BestCost)) {
            BestPenalty = CurrentPenalty;
            BestCost = Cost;
            RecordBetterTour();
            RecordBestTour();
            WriteTour(TourFileName, BestTour, BestCost);
        }
        OldOptimum = Optimum;
        if (MTSPObjective != MINMAX && MTSPObjective != MINMAX_SIZE) {
            if (CurrentPenalty == 0 && Cost < Optimum) Optimum = Cost;
        } else if (CurrentPenalty < Optimum)
            Optimum = CurrentPenalty;
        if (Optimum < OldOptimum) {
            printff("*** New OPTIMUM = " GainFormat " ***\n", Optimum);
            if (FirstNode->InputSuc) {
                Node *N = FirstNode;
                while ((N = N->InputSuc = N->Suc) != FirstNode)
                    ;
            }
        }
        Time = fabs(GetTime() - LastTime);
        UpdateStatistics(Cost, Time);
        if (TraceLevel >= 1 && Cost != PLUS_INFINITY) {
            printff("Run %d: ", Run);
            StatusReport(Cost, LastTime, "");
            printff("\n");
        }
        if (StopAtOptimum && MaxPopulationSize >= 1) {
            Runs = Run;
            break;
        }
        if (PopulationSize >= 2 && (PopulationSize == MaxPopulationSize || Run >= 2 * MaxPopulationSize) && Run < Runs) {
            Node *N;
            int Parent1, Parent2;
            Parent1 = LinearSelection(PopulationSize, 1.25);
            do Parent2 = LinearSelection(PopulationSize, 1.25);
            while (Parent2 == Parent1);
            ApplyCrossover(Parent1, Parent2);
            N = FirstNode;
            do {
                int d = C(N, N->Suc);
                AddCandidate(N, N->Suc, d, INT_MAX);
                AddCandidate(N->Suc, N, d, INT_MAX);
                N = N->InitialSuc = N->Suc;
            } while (N != FirstNode);
        }
        SRandom(++Seed);

        /* SPH Phase here!! */
    }
    PrintStatistics();
    if (Salesmen > 1) {
        if (Dimension == DimensionSaved) {
            for (i = 1; i <= Dimension; i++) {
                N = &NodeSet[BestTour[i - 1]];
                (N->Suc = &NodeSet[BestTour[i]])->Pred = N;
            }
        } else {
            for (i = 1; i <= DimensionSaved; i++) {
                Node *N1 = &NodeSet[BestTour[i - 1]];
                Node *N2 = &NodeSet[BestTour[i]];
                Node *M1 = &NodeSet[N1->Id + DimensionSaved];
                Node *M2 = &NodeSet[N2->Id + DimensionSaved];
                (M1->Suc = N1)->Pred = M1;
                (N1->Suc = M2)->Pred = N1;
                (M2->Suc = N2)->Pred = M2;
            }
        }
        CurrentPenalty = BestPenalty;
        MTSP_Report(BestPenalty, BestCost);
        MTSP_WriteSolution(MTSPSolutionFileName, BestPenalty, BestCost);
        SINTEF_WriteSolution(SINTEFSolutionFileName, BestCost);
    }

    printff("Best %s solution:\n", Type);
    CurrentPenalty = BestPenalty;
    SOP_Report(BestCost);

    printff("\n");

    return EXIT_SUCCESS;
}
