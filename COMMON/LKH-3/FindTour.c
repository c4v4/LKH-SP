#include "LKH.h"
#include "SimulatedAnnealing.h"
/*
 * After the candidate set has been created the FindTour function is called
 * a predetermined number of times (Runs).
 *
 * FindTour performs a number of trials, where in each trial it attempts
 * to improve a chosen initial tour using the modified Lin-Kernighan edge
 * exchange heuristics.
 *
 * Each time a better tour is found, the tour is recorded, and the candidates
 * are reorderded by the AdjustCandidateSet function. Precedence is given to
 * edges that are common to two currently best tours. The candidate set is
 * extended with those tour edges that are not present in the current set.
 * The original candidate set is re-established at exit from FindTour.
 */


static void SwapCandidateSets();
static GainType OrdinalTourCost;

GainType FindTour() {
    GainType Cost;
    Node *t;
    int i;
    double EntryTime = GetTime();
    double LastImprTime = EntryTime;
    double SaTimeLimit = /* Run == 0 ? RunTimeLimit / 2 : */ RunTimeLimit;
    if (SaTimeLimit < TimeLimit - (EntryTime - StartTime))
        SA_setup(0.0, SaTimeLimit);
    else
        SA_setup(EntryTime - StartTime, TimeLimit);

    t = FirstNode;
    do
        t->OldPred = t->OldSuc = t->NextBestSuc = t->BestSuc = 0;
    while ((t = t->Suc) != FirstNode);
    if (Run == 1 && Dimension == DimensionSaved) {
        OrdinalTourCost = 0;
        for (i = 1; i < Dimension; i++)
            OrdinalTourCost += C(&NodeSet[i], &NodeSet[i + 1]) - NodeSet[i].Pi - NodeSet[i + 1].Pi;
        OrdinalTourCost += C(&NodeSet[Dimension], &NodeSet[1]) - NodeSet[Dimension].Pi - NodeSet[1].Pi;
        OrdinalTourCost /= Precision;
    }
    BetterCost = PLUS_INFINITY;
    BetterPenalty = CurrentPenalty = PLUS_INFINITY;
    if (MaxTrials > 0)
        HashInitialize(HTable);
    else {
        Trial = 1;
        ChooseInitialTour();
        CurrentPenalty = PLUS_INFINITY;
        CurrentPenalty = BetterPenalty = Penalty();
    }
    for (Trial = 1; Trial <= MaxTrials; Trial++) {
        double now = GetTime();
        if (now - EntryTime >= RunTimeLimit) {
            if (TraceLevel >= 1)
                printff("*** Run time limit exceeded ***\n");
            break;
        }
        if (now - StartTime >= TimeLimit) {
            if (TraceLevel >= 1)
                printff("*** LKH time limit exceeded ***\n");
            break;
        }
        if ((Run == 0) && (now - LastImprTime > RunTimeLimit / 10.0)) {
            if (TraceLevel >= 1)
                printff("*** First Run early exit: no improvements for %.1f sec. ***\n", now - LastImprTime);
            break;
        }
        /* if (now - EntryTime >= RunTimeLimit / 2.0)
            FreezeSalesmen(); */
        /* Delayed SA start to normalize temperature with BetterCost */
        if (Trial == 5)
            SA_start();
        /* Choose FirstNode at random */
        if (Dimension == DimensionSaved)
            FirstNode = &NodeSet[1 + Random() % Dimension];
        else
            for (i = Random() % Dimension; i > 0; i--)
                FirstNode = FirstNode->Suc;
        ChooseInitialTour();
        if ((ProblemType == SOP || ProblemType == M1_PDTSP) && (InitialTourAlgorithm != SOP_ALG || Trial > 1))
            SOP_RepairTour();
        Cost = LinKernighan();
        if (FirstNode->BestSuc && !TSPTW_Makespan) {
            /* Merge tour with current best tour */
            t = FirstNode;
            while ((t = t->Next = t->BestSuc) != FirstNode)
                ;
            Cost = MergeWithTour();
        }
        if (Dimension == DimensionSaved && Cost >= OrdinalTourCost && BetterCost > OrdinalTourCost && !TSPTW_Makespan) {
            /* Merge tour with ordinal tour */
            for (i = 1; i < Dimension; i++)
                NodeSet[i].Next = &NodeSet[i + 1];
            NodeSet[Dimension].Next = &NodeSet[1];
            Cost = MergeWithTour();
        }
        if (ExtractRoutes(Cost))
            LastImprTime = now;
        if (SA_test(CurrentPenalty, Cost)) {
            if (TraceLevel >= 1) {
                printff("* %d: ", Trial);
                StatusReport(Cost, EntryTime, "");
            }
            BetterCost = Cost;
            BetterPenalty = CurrentPenalty;
            RecordBetterTour();
            if (StopAtOptimum) {
                if (ProblemType != CCVRP && ProblemType != TRP && ProblemType != MLP && MTSPObjective != MINMAX &&
                            MTSPObjective != MINMAX_SIZE
                        ? CurrentPenalty == 0 && Cost == Optimum
                        : CurrentPenalty == Optimum)
                    break;
            }
            AdjustCandidateSet();
            HashInitialize(HTable);
            HashInsert(HTable, Hash, Cost);
        } else if (TraceLevel >= 2) {
            printff("  %d: ", Trial);
            StatusReport(Cost, EntryTime, "");
        }
        /* Record backbones if wanted */
        if (Trial <= BackboneTrials && BackboneTrials < MaxTrials) {
            SwapCandidateSets();
            AdjustCandidateSet();
            if (Trial == BackboneTrials) {
                if (TraceLevel >= 1) {
                    printff("# %d: Backbone candidates ->\n", Trial);
                    CandidateReport();
                }
            } else
                SwapCandidateSets();
        }
    }
    if (BackboneTrials > 0 && BackboneTrials < MaxTrials) {
        if (Trial > BackboneTrials || (Trial == BackboneTrials && (!StopAtOptimum || BetterCost != Optimum)))
            SwapCandidateSets();
        t = FirstNode;
        do {
            free(t->BackboneCandidateSet);
            t->BackboneCandidateSet = 0;
        } while ((t = t->Suc) != FirstNode);
    }
    t = FirstNode;
    if (Norm == 0 || MaxTrials == 0 || !t->BestSuc) {
        do
            t = t->BestSuc = t->Suc;
        while (t != FirstNode);
    }
    Hash = 0;
    do {
        (t->Suc = t->BestSuc)->Pred = t;
        Hash ^= Rand[t->Id] * Rand[t->Suc->Id];
    } while ((t = t->BestSuc) != FirstNode);
    if (Trial > MaxTrials)
        Trial = MaxTrials;
    ResetCandidateSet();
    /* UnfreezeSalesmen(); */
    CurrentPenalty = BetterPenalty;
    return BetterCost;
}

/*
 * The SwapCandidateSets function swaps the normal and backbone candidate sets.
 */

static void SwapCandidateSets() {
    Node *t = FirstNode;
    do {
        Candidate *Temp = t->CandidateSet;
        t->CandidateSet = t->BackboneCandidateSet;
        t->BackboneCandidateSet = Temp;
    } while ((t = t->Suc) != FirstNode);
}
