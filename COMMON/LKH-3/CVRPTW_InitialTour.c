#include "LKH.h"

#define TWIN(N) (N + DimensionSaved)

typedef struct NDepotCost_ {
    Node *N;
    double Cost;
} NDepotCost;

static double max(double a, double b) { return a > b ? a : b; }

static int compareEarliest(const void *n1, const void *n2) {
    double f1 = ((NDepotCost *)n1)->Cost;
    double f2 = ((NDepotCost *)n2)->Cost;
    return (f1 > f2) - (f1 < f2);
}

static Node **SelectSeeds(NDepotCost *SortableNodes) {
    Node *StartN = SortableNodes[0].N;
    Node *N = StartN;
    Node **Routes = (Node **)malloc(sizeof(Node *) * Salesmen);
    assert(Routes);
    for (int i = 0; i < Salesmen; ++i) {
        while (/* rand() & 1 ||  */ N->V)
            N = N->Suc;
        Node *NextN = N->Suc;
        N->V = 1;
        Link(N->Pred, N->Suc);
        N->Pred = N->Suc = NULL;
        Routes[i] = N;
        N = NextN;
    }

    return Routes;
}

static void ConstructTour(Node **Routes) {
    Node *N = NULL;
    Node *PredN = NULL;
    Node *Dep = Depot;
    for (int i = 0; i < Salesmen; ++i) {
        N = Routes[i];
        do {
            PredN = N->Pred;
            Link(N->Pred, TWIN(N));
            Link(TWIN(N), N);
        } while ((N = PredN)->Pred);
        Link(Routes[(i + 1) % Salesmen], TWIN(Dep));
        Link(TWIN(Dep), Dep);
        Link(Dep, TWIN(N));
        Link(TWIN(N), N);
        Dep = &NodeSet[Dim + i + 1];
    }
}

GainType CVRPTW_InitialTour() {
    double EntryTime = GetTime();
    int Customers = Dim - 1;

    NDepotCost *SortableNodes = (NDepotCost *)malloc(sizeof(NDepotCost) * Customers);
    assert(SortableNodes);

    for (int i = 0; i < Customers; ++i) {
        Node *N = &NodeSet[i + 2];
        SortableNodes[i].N = N;
        SortableNodes[i].Cost = N->Latest;
    }

    qsort(SortableNodes, Customers, sizeof(NDepotCost), compareEarliest);
Start:
    for (int i = 0; i < Customers; ++i) {
        Node *N = &NodeSet[i + 2];
        N->V = 0;
        N->prevDemandSum = N->Demand;
        N->prevCostSum = max(OldDistance(Depot, TWIN(N)), N->Earliest) + N->ServiceTime;
    }

    for (int i = 0; i < Customers - 1; ++i)
        Link(SortableNodes[i].N, SortableNodes[i + 1].N);
    Link(SortableNodes[Dim - 2].N, SortableNodes[0].N);

    Node **Routes = SelectSeeds(SortableNodes);
    int NodesLeft = Dim - 1 - Salesmen;

    printff("Seed nodes:\n");
    for (int i = 0; i < Salesmen; ++i) {
        printff("Route #%d: %d (CostSum: " GainFormat ")\n", i, Routes[i]->Id, Routes[i]->prevCostSum);
    }

    int i = Customers - 1;
    do
        FirstNode = SortableNodes[--i].N;
    while (FirstNode->V == 1);
    assert(i >= 0);

    for (; NodesLeft > 0; --NodesLeft) {

        i = Customers - 1;
        while (FirstNode->V == 1) {
            FirstNode = SortableNodes[i--].N;
            assert(i >= 0);
        }

        Node *N = FirstNode;
        Node *CN = N, *LN = N, *GN = N; /* Node with the smallest CostSum if added to CR */
        int CR = -1, LR = -1, GR = -1;  /* Route index where CN can be attached */
        GainType CostSum = PLUS_INFINITY;
        double GlobTimeFromLatest = PLUS_INFINITY;
        int Cap = Capacity;
        do {
            // printff("Node %d, V: %d\n", N->Id, N->V);
            if (!N->V) {
                double TimeFromLatest = MINUS_INFINITY;
                for (int i = 0; i < Salesmen; ++i) {
                    double CandCostSum = Routes[i]->prevCostSum + OldDistance(Routes[i], TWIN(N));
                    if (CandCostSum < N->Earliest)
                        CandCostSum = N->Earliest;

                    if (N->Latest - CandCostSum > TimeFromLatest) {
                        TimeFromLatest = N->Latest - CandCostSum;
                        LN = N;
                        LR = i;
                    }

                    CandCostSum += N->ServiceTime;
                    CandCostSum += OldDistance(N, TWIN(Depot));

                    if (CandCostSum < CostSum && Routes[i]->prevDemandSum + N->Demand <= Cap) {
                        CostSum = CandCostSum;
                        CN = N;
                        CR = i;
                    }
                }

                if (GlobTimeFromLatest > TimeFromLatest) {
                    GlobTimeFromLatest = TimeFromLatest;
                    GN = LN;
                    GR = LR;
                }
            }
            N = N->Suc;
            assert(!(Cap == INT_MAX && N == FirstNode));
            if (N == FirstNode) {
                if (CR == -1) {
                    Cap = INT_MAX; /* Relax capacity constraint */
                    printff("Relaxed Capacity Constraint\n");
                } else
                    break;
            }
        } while (1);

        if (GlobTimeFromLatest < 3000) {
            printff("Node %d placed in route %d because was %.0f from latest\n", GN->Id, GR, GlobTimeFromLatest);
            if (GlobTimeFromLatest < 0) {
                if (!Relocate(Routes, GN)) {
                    int i = 0;
                    while (SortableNodes[i].N != GN)
                        ++i;
                    int NewPos = rand() % Salesmen;
                    NDepotCost tmp = SortableNodes[NewPos];
                    SortableNodes[NewPos] = SortableNodes[i];
                    SortableNodes[i] = tmp;
                    goto Start;
                } else {
                    continue;
                }
            }
            CostSum = GN->Latest - GlobTimeFromLatest + GN->ServiceTime + OldDistance(GN, TWIN(Depot));
            CN = GN;
            CR = GR;
        }

        printff("Node %d assigned to route %d. CostSum " GainFormat "\n", CN->Id, CR, CostSum);
        Link(CN->Pred, CN->Suc);
        Link(Routes[CR], CN);
        CN->Suc = NULL;
        CN->V = 1;
        CN->prevCostSum = CostSum - OldDistance(CN, TWIN(Depot));
        CN->prevDemandSum = Routes[CR]->prevDemandSum + CN->Demand;
        Routes[CR] = CN;
    }

    printff("Final Routes:\n");
    for (int i = 0; i < Salesmen; ++i) {
        printff("Route #%d: ", i);
        Node *N = Routes[i];
        while (N) {
            printff("%d[%.0f (%.0f) %.0f] <-- ", N->Id, N->Earliest, N->prevCostSum - N->ServiceTime, N->Latest);
            N = N->Pred;
        }
        printff("\n");
    }

    ConstructTour(Routes);

    /* Print initial tour info */
    FirstNode = Depot;
    Node *N = FirstNode, *NextN;
    GainType Cost = 0;
    do {
        NextN = N->Suc;
        Cost += (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
        N = NextN->Suc;
    } while ((N = N->Suc) != FirstNode);
    CurrentPenalty = Penalty();
    printff("CVRPTW = " GainFormat "_" GainFormat "", CurrentPenalty, Cost);
    printff(", Time = %0.2f sec.\n", fabs(GetTime() - EntryTime));
    free(SortableNodes);
    free(Routes);

    return Cost;
}


int Relocate(Node **Routes, Node *R) {

    static Node *StartRoute = 0;
    Node *N, *NextN, *CurrentRoute;
    GainType CostSum, DemandSum;

    for (int i = 0; i < Salesmen; ++i) {
        N = Routes[i];
        while (N->Pred)
            N = N->Pred;
        Node *StartN = N;
        CostSum = OldDistance(Depot, TWIN(N));
        DemandSum = 0;

        Node *bestPosition = NULL;
        GainType CostSumIncrease = -OldDistance(Depot, TWIN(N));
        CostSumIncrease += OldDistance(Depot, TWIN(R));
        if (CostSumIncrease < R->Earliest)
            CostSumIncrease = R->Earliest;
        if (CostSum + CostSumIncrease <= R->Latest) {
            CostSumIncrease += R->ServiceTime;
            CostSumIncrease += OldDistance(R, TWIN(N));
        }

        int feasible = N->prevCostSum + CostSumIncrease < N->Latest;
        while (N) {
            DemandSum += N->Demand;
            if (CostSum + CostSumIncrease < N->Earliest)
                CostSumIncrease = N->Earliest - CostSum;
            if (CostSum < N->Earliest)
                CostSum = N->Earliest;
            feasible &= CostSum + CostSumIncrease <= N->Latest;
            // printff(GainFormat " " GainFormat " | %d = " GainFormat " < %.0f\n", CostSum, N->prevCostSum, feasible,
            //        CostSum + CostSumIncrease, N->Latest);
            CostSum += N->ServiceTime;
            assert(CostSum == N->prevCostSum);

            NextN = N->Suc;
            if (NextN) {
                GainType CNNextN = OldDistance(N, TWIN(NextN));
                CostSum += CNNextN;

                GainType cost_inc = -CNNextN;
                cost_inc += OldDistance(N, TWIN(R));
                if (CostSum + cost_inc < R->Earliest)
                    cost_inc = R->Earliest - CostSum;
                if (CostSum + cost_inc <= R->Latest) {
                    cost_inc += R->ServiceTime;
                    cost_inc += OldDistance(R, TWIN(NextN));
                    if (cost_inc < CostSumIncrease) {
                        CostSumIncrease = cost_inc;
                        bestPosition = N;
                        feasible = CostSum + CostSumIncrease <= NextN->Latest;
                        // if (feasible)
                        //    printff("Found (maybe) feasible position betwen %d and %d (" GainFormat " < %.0f)\n", N->Id, NextN->Id,
                        //            CostSum + CostSumIncrease, NextN->Latest);
                    }
                }
            }

            N = NextN;
        }

        if (R->Demand + DemandSum < Capacity && feasible) {
            printff("Relocate %d in route %d\n", R->Id, i);
            Link(R->Pred, R->Suc);
            if (bestPosition) {
                N = bestPosition;
                NextN = N->Suc;
                Link(N, R);
                Link(R, NextN);
                N = StartN;
            } else {
                Link(R, StartN);
                R->Pred = NULL;
                N = R;
            }
            R->V = 1;
            DemandSum = 0;
            CostSum = 0;
            Node *PrevN = Depot;
            do {
                DemandSum += N->Demand;
                CostSum += OldDistance(PrevN, TWIN(N));
                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;
                //assert(CostSum <= N->Latest);
                CostSum += N->ServiceTime;
                N->prevDemandSum = DemandSum;
                N->prevCostSum = CostSum;
                PrevN = N;
            } while ((N = N->Suc));
            return 1;
        }
    }
    return 0;
}