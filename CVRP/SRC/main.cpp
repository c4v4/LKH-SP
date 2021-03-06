
extern "C" {

#include "BIT.h"
#include "Genetic.h"
#include "LKH.h"
}

#include <vector>

#define VERBOSE
#define VERBOSE_LEVEL 1

#define NDEBUG
#include "SPH.hpp"
#undef NDEBUG

#include <string>

sph::SPHeuristic *sph_ptr;          /* SPHeuristc pointer */
std::vector<sph::idx_t> BestRoutes; /* Vector containing routes indexes of BestTour */

void print_sol(sph::Instance &inst, sph::GlobalSolution &sol) {
    int route = 1;
    for (sph::idx_t j : sol) {
        fmt::print("Route #{}: ", route++);
        for (sph::idx_t i : inst.get_col(j))
            fmt::print("{} ", i + 1);
        fmt::print("\n");
    }
    fmt::print("Cost {}\n", sol.get_cost());
    fflush(stdout);
}

void restart() {
    Node *N = Depot;
    SalesmenUsed = 0;
    do {
        int Size = 0;
        while ((N = N->Suc)->DepotId == 0)
            Size++;
        SalesmenUsed += Size > 0;
    } while (N != Depot);
    Salesmen = SalesmenUsed;
    Trial = 0;
    MTSPMinSize = 1;
    ReadProblem();
    AllocateStructures();
    CreateCandidateSet();
    InitializeStatistics();
}

char *default_params(char *argv0) {
    char *Buffer;

    std::string bin_path(argv0);
    while (bin_path.back() != '/')
        bin_path.pop_back();
    bin_path += "default_params.txt";
    Buffer = (char *)malloc(bin_path.size() + 1);
    strcpy(Buffer, bin_path.c_str());
    return Buffer;
}

int main(int argc, char *argv[]) {
    GainType Cost, OldOptimum;
    double Time, LastTime;
    Node *N;
    int i;

    for (i = 0; i < argc; i++)
        printff("%s ", argv[i]);
    printff("\n");

    if (argc == 1) {
        printff("Usage: ./cvrp <instance-file> [<time-limit>] [<param-file>]\n");
        return EXIT_FAILURE;
    }

    SetDefaultParameters();
    if (argc > 1)
        ProblemFileName = argv[1];
    if (argc > 2)
        if (argv[2][0] == '0')
            Scale = 1000;
    if (argc > 3)
        TimeLimit = atoi(argv[3]);
    if (argc > 4)
        ParameterFileName = argv[4];
    else
        ParameterFileName = default_params(argv[0]);

    ReadParameters();
    if (InitialSolFileName)
        Salesmen = GetSalesmentFromSolFile(InitialSolFileName);
    else if (Salesmen == 0)
        MTSPMinSize = 0;

    StartTime = LastTime = GetTime();
    MergeWithTour = Recombination == IPT ? MergeWithTourIPT : MergeWithTourGPX2;
    OutputSolFile = stdout;
    ReadProblem();
    if (InitialSolFileName)
        Read_InitialTour_Sol(InitialSolFileName);

    int *warmstart = (int *)malloc((DimensionSaved + 1) * sizeof(int));
    assert(warmstart);

    if (TraceLevel >= 1) {
        printff("done\n");
        PrintParameters();
    } else
        printff("PROBLEM_FILE = %s\n", ProblemFileName ? ProblemFileName : "");

    AllocateStructures();
    CreateCandidateSet();
    InitializeStatistics();

    Norm = 9999;
    BestCost = PLUS_INFINITY;
    BestPenalty = CurrentPenalty = PLUS_INFINITY;

    sph::SPHeuristic sph(Dim - 1);
    sph.set_new_best_callback(print_sol);
    sph.set_max_routes(500'000U);
    sph.set_keepcol_strategy(sph::SPP);
    sph_ptr = &sph;

    double Tlim = (TimeLimit - GetTime() + StartTime); /* Remaining time */
    if (SphTimeLimit == DBL_MAX)
        SphTimeLimit = Tlim / 30;
    if (RunTimeLimit == DBL_MAX)
        RunTimeLimit = Tlim / 10;

    /* Find a specified number (Runs) of local optima */
    for (Run = 1; Run <= Runs; Run++) {
        LastTime = GetTime();
        if (LastTime - StartTime >= TimeLimit) {
            if (TraceLevel >= 1)
                printff("*** Time limit exceeded ***\n");
            break;
        }
        printff("Run time limit: %g sec., remaining Time: %g sec.\n", RunTimeLimit, TimeLimit - LastTime + StartTime);
        Cost = FindTour(); /* using the Lin-Kernighan heuristic */
        if (MaxPopulationSize > 1 && !TSPTW_Makespan) {
            /* Genetic algorithm */
            int i;
            for (i = 0; i < PopulationSize; i++) {
                GainType OldPenalty = CurrentPenalty;
                GainType OldCost = Cost;
                Cost = MergeTourWithIndividual(i);
                if (TraceLevel >= 1 && (CurrentPenalty < OldPenalty || (CurrentPenalty == OldPenalty && Cost < OldCost))) {
                    if (CurrentPenalty)
                        printff("  Merged with %d: Cost = " GainFormat, i + 1, Cost);
                    else
                        printff("  Merged with %d: Cost = " GainFormat "_" GainFormat, i + 1, CurrentPenalty, Cost);
                    if (Optimum != MINUS_INFINITY && Optimum != 0) {
                        printff(", Gap = %0.4f%%", 100.0 * (Cost - Optimum) / Optimum);
                    }
                    printff("\n");
                }
            }
            if (!HasFitness(CurrentPenalty, Cost)) {
                if (PopulationSize < MaxPopulationSize) {
                    AddToPopulation(CurrentPenalty, Cost);
                    if (TraceLevel >= 1)
                        PrintPopulation();
                } else if (SmallerFitness(CurrentPenalty, Cost, PopulationSize - 1)) {
                    i = ReplacementIndividual(CurrentPenalty, Cost);
                    ReplaceIndividualWithTour(i, CurrentPenalty, Cost);
                    if (TraceLevel >= 1)
                        PrintPopulation();
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
            WriteSolFile(BestTour, BestCost, NULL);
        }
        OldOptimum = Optimum;
        if (MTSPObjective != MINMAX && MTSPObjective != MINMAX_SIZE) {
            if (CurrentPenalty == 0 && Cost < Optimum)
                Optimum = Cost;
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
        if (PopulationSize >= 2 && (PopulationSize == MaxPopulationSize || Run >= 2 * MaxPopulationSize) && Run < Runs) {
            Node *N;
            int Parent1, Parent2;
            Parent1 = LinearSelection(PopulationSize, 1.25);
            do
                Parent2 = LinearSelection(PopulationSize, 1.25);
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

        /* Set Partitioning Heuristic phase */
        if (Run && Run % SphPeriod == 0) {
            sph.set_timelimit(SphTimeLimit);
            sph.set_ncols_constr(BestRoutes.size());
            auto BestRCopy = BestRoutes;
            BestRoutes = sph.solve(BestRoutes);
            if (!BestRoutes.empty()) { /* Transform back SP sol to tour*/
                GainType Cost = 0;
                int *ws = warmstart + 1;
                for (sph::idx_t j : BestRoutes) {
                    sph::Column &col = sph.get_col(j);
                    Cost += col.get_cost() * Scale;
                    *ws++ = MTSPDepot;
                    for (sph::idx_t &i : col) {
                        if (ws - warmstart > DimensionSaved + 1)
                            eprintf("%s, Error SPH: Solution too long!\n", ProblemFileName);
                        *ws++ = i + 2;
                    }
                }
                for (sph::idx_t j = BestRoutes.size(); j < Salesmen; ++j)
                    *ws++ = MTSPDepot;
                warmstart[0] = warmstart[DimensionSaved];
                WriteSolFile(warmstart, Cost, NULL);
                SetInitialTour(warmstart);
            }
            RunTimeLimit *= 2;
        }
        if (Run == 1 && MTSPMinSize == 0)
            restart();
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

    FreeStructures();
    free(warmstart);
    if (Runs == 1)
        return SalesmenUsed;

    return EXIT_SUCCESS;
}
