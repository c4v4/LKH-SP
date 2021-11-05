#include "LKH.h"

#define TWIN(N) (N + DimensionSaved)

#define OldDistance(N1, N2) ((C(N1, N2) - N1->Pi - N2->Pi) / Precision)

typedef struct NDepotCost_ {
    Node *N;
    double Cost;
} NDepotCost;

static double max(double a, double b) { return a > b ? a : b; }

static int compareCost(const void *n1, const void *n2) {
    double f1 = ((NDepotCost *)n1)->Cost;
    double f2 = ((NDepotCost *)n2)->Cost;
    return (f1 > f2) - (f1 < f2);
}

static void SelectSeeds(NDepotCost *Seeds, Node **Routes, int Veh) {
    for (int i = 0; i < Dim - 2 && Veh; ++i) {
        Node *N = Seeds[i].N;
        while (/* rand() & 1 ||  */ N->V)
            N = Seeds[++i].N;
        N->V = 1;
        Link(N->Pred, N->Suc);
        N->Pred = N->Suc = NULL;
        Routes[--Veh] = N;
    }
}

static void ConstructTour(Node **Routes, int Veh) {
    Node *N = NULL;
    Node *PredN = NULL;
    Node *Dep = Depot;
    int i = 0;
    for (; i < Veh; ++i) {
        N = PredN = Routes[i];
        while ((PredN = N->Pred)) {
            Link(N->Pred, TWIN(N));
            Link(TWIN(N), N);
            N = PredN;
        }
        if (i != Veh - 1)
            Link(Routes[(i + 1)], TWIN(Dep));
        Link(TWIN(Dep), Dep);
        Link(Dep, TWIN(N));
        Link(TWIN(N), N);
        Dep = &NodeSet[Dim + i + 1];
    }
    for (; i < Salesmen; ++i) {
        N = &NodeSet[Dim + i - 1];
        Dep = &NodeSet[Dim + i];
        Link(TWIN(Dep), Dep);
        Link(Dep, TWIN(N));
    }
    Link(Routes[0], TWIN(Dep));
}

int tentative_routes(Node **Routes, NDepotCost *Seeds, NDepotCost *CostSumLB, int Veh) {
    int Customers = Dim - 1;

    for (int i = 0; i < Customers; ++i) {
        Node *N = &NodeSet[i + 2];
        N->V = 0;
        N->prevDemandSum = N->Demand;
        N->prevCostSum = max(OldDistance(Depot, TWIN(N)), N->Earliest) + N->ServiceTime;
    }

    for (int i = 0; i < Customers - 1; ++i)
        Link(CostSumLB[i].N, CostSumLB[i + 1].N);
    Link(CostSumLB[Customers - 1].N, CostSumLB[0].N);

    assert(Veh <= Salesmen);
    SelectSeeds(Seeds, Routes, Veh);
    int NodesLeft = Customers - Veh;

    printff("Seed nodes: (%d)\n", Veh);
    // for (int i = 0; i < Veh; ++i) {
    //    printff("Route #%d: %d (CostSum: " GainFormat ", Latest: %.0f, DepotDist: %d)\n", i, Routes[i]->Id, Routes[i]->prevCostSum,
    //            Routes[i]->Latest, OldDistance(Depot, TWIN(Routes[i])));
    //}

    int i;
    for (; NodesLeft > 0; --NodesLeft) {
        i = 0;
        while (CostSumLB[i].N->V == 1)
            ++i;
        int CN = -1; /* Node with the smallest CostSum if added to CR */
        int GN = -1; /* Node closest to time-infeasibility */
        int CR = -1; /* Route index where CN can be attached */
        int GR = -1; /* Route index where GN can be attached */
        GainType CostSum = PLUS_INFINITY;
        double GlobTimeFromLatest = (double)PLUS_INFINITY;
        int Cap = Capacity;
        Node *N;
        do {
            N = CostSumLB[i++].N;
            if (!N->V) {
                int LN = -1;
                int LR = -1;
                double TimeFromLatest = MINUS_INFINITY;
                for (int j = 0; j < Veh; ++j) {
                    double CandCostSum = Routes[j]->prevCostSum + OldDistance(Routes[j], TWIN(N));
                    if (CandCostSum < N->Earliest)
                        CandCostSum = N->Earliest;
                    if (N->Latest - CandCostSum > TimeFromLatest) {
                        TimeFromLatest = N->Latest - CandCostSum;
                        LN = i - 1;
                        LR = j;
                    }
                    CandCostSum += N->ServiceTime;
                    CandCostSum += OldDistance(N, TWIN(Depot));
                    if (CandCostSum < CostSum /* && Routes[j]->prevDemandSum + N->Demand <= Cap */) {
                        CostSum = CandCostSum;
                        CN = i - 1;
                        CR = j;
                    }
                }
                if (LN >= 0 && GlobTimeFromLatest > TimeFromLatest /* && Routes[LR]->prevDemandSum + LN->Demand <= Cap */) {
                    GlobTimeFromLatest = TimeFromLatest;
                    GN = LN;
                    GR = LR;
                }
            }
            /*  if (!CN) {
                 if (i == Customers) {
                     i = 0;
                     while (CostSumLB[i].N->V == 1)
                         ++i;
                     Cap = INT_MAX; //Relax capacity constraint
                     printff("Relaxed Capacity Constraint\n");
                 } else
                     return 0;
             } */
        } while (i < Customers);

        if (GlobTimeFromLatest < 0 || CostSum > Depot->Latest || CR == -1) { /* Best insertion is infeasible */
            if (GN >= 0) {
                printff("Node %d cannot be placed in a route (Time from latest = %.0f, Cost sum = %lld <?> %.0f)\n", CostSumLB[GN].N->Id,
                        GlobTimeFromLatest, CostSum, Depot->Latest);
                int seed_i = 0;
                while (Seeds[seed_i].N != CostSumLB[GN].N)
                    ++seed_i;
                int tmp_i = /*  rand() %  */ Veh;
                NDepotCost tmp = Seeds[tmp_i];
                Seeds[tmp_i] = Seeds[seed_i];
                Seeds[seed_i] = tmp;
            } else {
                printff("Candidate node not found(nodes left %d).\n", NodesLeft);
                for (int j = 0; j < Veh; ++j) {
                    printff("Route #%d: %d (DemandSum: " GainFormat ", CostSum: " GainFormat ", Latest: %.0f, DepotDist: %d)\n", j,
                            Routes[j]->Id, Routes[j]->prevDemandSum, Routes[j]->prevCostSum, Routes[j]->Latest,
                            OldDistance(Depot, TWIN(Routes[j])));
                }
            }
            return 0;
        }

        if (GlobTimeFromLatest < 5000) {
            // printff("Node %d placed in route %d because was %.0f from latest\n", GN->Id, GR, GlobTimeFromLatest);
            CostSum = CostSumLB[GN].N->Latest - GlobTimeFromLatest + CostSumLB[GN].N->ServiceTime +
                      OldDistance(CostSumLB[GN].N, TWIN(Depot));
            CN = GN;
            CR = GR;
        }

        // printff("Node %d assigned to route %d. CostSum " GainFormat "\n", CN->Id, CR, CostSum);
        Link(CostSumLB[CN].N->Pred, CostSumLB[CN].N->Suc);
        Link(Routes[CR], CostSumLB[CN].N);
        CostSumLB[CN].N->Suc = NULL;
        CostSumLB[CN].N->V = 1;
        CostSumLB[CN].N->prevCostSum = CostSum - OldDistance(CostSumLB[CN].N, TWIN(Depot));
        CostSumLB[CN].N->prevDemandSum = Routes[CR]->prevDemandSum + CostSumLB[CN].N->Demand;
        Routes[CR] = CostSumLB[CN].N;
    }
    return 1;
}

GainType CVRPTW_InitialTourOld() {
    double EntryTime = GetTime();
    int Customers = Dim - 1;

    NDepotCost *CostSumLB = (NDepotCost *)malloc(sizeof(NDepotCost) * Customers);
    assert(CostSumLB);
    NDepotCost *Seeds = (NDepotCost *)malloc(sizeof(NDepotCost) * Customers);
    assert(Seeds);
    Node **Routes = (Node **)malloc(sizeof(Node *) * Salesmen);
    assert(Routes);

    for (int i = 0; i < Customers; ++i) {
        Node *N = &NodeSet[i + 2];
        CostSumLB[i].N = Seeds[i].N = N;
        CostSumLB[i].Cost = max(OldDistance(Depot, TWIN(N)), N->Earliest) + N->ServiceTime + OldDistance(N, TWIN(Depot));
        Seeds[i].Cost = N->Latest - OldDistance(Depot, TWIN(N));
    }
    qsort(CostSumLB, Customers, sizeof(NDepotCost), compareCost);
    qsort(Seeds, Customers, sizeof(NDepotCost), compareCost);

    /* int feasVeh = Salesmen;
    int failVeh = 0;
    int Veh;
    while (feasVeh != failVeh + 1) {
        Veh = (feasVeh + 3 * failVeh + 3) / 4;
        if (tentative_routes(Routes, Seeds, CostSumLB, Veh)) {
            feasVeh = Veh;
        } else {
            failVeh = Veh;
        }
    }
    while (tentative_routes(Routes, Seeds, CostSumLB, --Veh))
        ;
    while (!tentative_routes(Routes, Seeds, CostSumLB, Veh))
        ; */
    int Veh = 10;
    while (!tentative_routes(Routes, Seeds, CostSumLB, Veh))
        ++Veh;

    printff("Final Routes:\n");
    for (int i = 0; i < Veh; ++i) {
        printff("Route #%d: ", i);
        printff("%d[%.0f %.0f] <-- ", Depot->Id, Depot->Earliest, Depot->Latest);
        Node *N = Routes[i];
        while (N) {
            printff("%d[%.0f (%.0f) %.0f] <-- ", N->Id, N->Earliest, N->prevCostSum - N->ServiceTime, N->Latest);
            N = N->Pred;
        }
        printff("\n");
    }

    ConstructTour(Routes, Veh);

    /* Print initial tour info */
    FirstNode = Depot;
    Node *N = FirstNode, *NextN;
    GainType Cost = 0, CostSum = 0, DemandSum = 0;
    do {
        NextN = N->Suc;
        Cost += OldDistance(N, NextN);
        N = NextN->Suc;
    } while (N != FirstNode);
    CurrentPenalty = Penalty();
    printff("CVRPTW = " GainFormat "_" GainFormat "", CurrentPenalty, Cost);
    printff(", Time = %0.2f sec. (%d vehicles)\n", fabs(GetTime() - EntryTime), Veh);
    free(CostSumLB);
    free(Routes);
    free(Seeds);
    return Cost;
}


/* int Relocate(Node **Routes, Node *R) {

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
                // assert(CostSum <= N->Latest);
                CostSum += N->ServiceTime;
                N->prevDemandSum = DemandSum;
                N->prevCostSum = CostSum;
                PrevN = N;
            } while ((N = N->Suc));
            return 1;
        }
    }
    return 0;
} */