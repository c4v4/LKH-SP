#include "LKH.h"
#include "Segment.h"

static int Relocate(Node *R);

void MakeFeasible_CVRPTW() {
    if (ProblemType != CVRPTW)
        return;

    static Node *StartRoute = 0;
    Node *N, *NextN, *CurrentRoute;
    GainType CostSum, DemandSum;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;
    int cache_index = 0;

    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do {
        cava_NodeCache[cache_index++] = CurrentRoute = N;
        CostSum = DemandSum = 0;
        do {
            if (N->Id <= Dim && N != Depot) {
                if ((DemandSum + N->Demand) > Capacity)
                    if (Relocate(N)) {
                        N = CurrentRoute; /*TODO: no need to restart route-check, optimize*/
                        break;
                    }
                DemandSum += N->Demand;
                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;
                else if (CostSum > N->Latest) {
                    if (Relocate(N)) {
                        N = CurrentRoute; /*TODO: no need to restart route-check, optimize*/
                        break;
                    }
                }
                CostSum += N->ServiceTime;
            }
            NextN = Forward ? SUCC(N) : PREDD(N);
            CostSum += (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
            N = Forward ? SUCC(NextN) : PREDD(NextN);
        } while (N->DepotId == 0);
    } while (N != StartRoute);
}


int Relocate(Node *R) {
    if (ProblemType != CVRPTW)
        return 0;

    static Node *StartRoute = 0;
    Node *N, *NextN, *CurrentRoute;
    GainType CostSum, DemandSum;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;

    Node *RTwin = Forward ? PREDD(R) : SUCC(R);
    assert(RTwin == R + DimensionSaved);

    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do {
        CurrentRoute = N;
        CostSum = DemandSum = 0;
        Node *bestPosition = NULL;
        GainType CostSumIncrease = INT_MAX;
        int feasible = N->PetalId != R->PetalId; /* true if the relocation maintains the remaining part of the route feasible */
        do {
            if (N->Id <= Dim && N != Depot && N->PetalId != R->PetalId) {
                DemandSum += N->Demand;

                if (CostSum + CostSumIncrease < N->Earliest)
                    CostSumIncrease = N->Earliest - CostSum;

                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;

                feasible &= CostSum + CostSumIncrease <= N->Latest;
                CostSum += N->ServiceTime;
            }
            NextN = Forward ? SUCC(N) : PREDD(N);

            if (N->PetalId != R->PetalId) {
                GainType CNNextN = (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
                CostSum += CNNextN;

                GainType cost_inc = -CNNextN;
                cost_inc += (C(N, RTwin) - N->Pi - RTwin->Pi) / Precision;
                if (CostSum + cost_inc < R->Earliest)
                    cost_inc = R->Earliest - CostSum;
                if (CostSum + cost_inc <= R->Latest) {
                    cost_inc += R->ServiceTime;
                    cost_inc += (C(R, NextN) - R->Pi - NextN->Pi) / Precision;
                    if (cost_inc < CostSumIncrease) {
                        CostSumIncrease = cost_inc;
                        bestPosition = N;
                        feasible = 1;
                    }
                }
            }

            N = Forward ? SUCC(NextN) : PREDD(NextN);
        } while (N->DepotId == 0);

        if (R->Demand + DemandSum < Capacity && bestPosition && feasible) {
            N = bestPosition;
            NextN = Forward ? SUCC(N) : PREDD(N);
            /*
            Pun RTwin-->R between N and NextN
            --- Regular edges
            === Fixed edges

            0. Starting state:
            ===[1]---[2=RTwin]===[5=R]---[6]===


            ---[]===[3=N]---[t3=NextN]===[]---


            1. 2Opt: t1--t2 t3--t4
            =====[1]   [2]===[5]---[6]=====
                  \     |
                   `----|----.
                        |     \
            -----[]====[3]   [4]====[]-----


            2. 2Opt: t6--t5 t4--t1
            =====[1]   [2]===[5]   [6]=====
                  \     |     |     /
                   `----|-----|----'
                        |     |
            -----[]====[3]   [4]====[]-----
            */
            Node *t1 = Forward ? PREDD(RTwin) : SUCC(RTwin);
            Node *t2 = RTwin;
            Node *t3 = N;
            Node *t4 = NextN;
            Node *t5 = R;
            Node *t6 = Forward ? SUCC(R) : PREDD(R);
            GainType Gain = (C(t1, t2) + C(t3, t4) + C(t5, t6)) - (C(t2, t3) + C(t4, t5) + C(t1, t6));

            /*             Node *First;
                        printff("\nRelocating %d between %d and %d\n", R->Id, N->Id, NextN->Id);
                        printff("Before:\n");
                        if (Forward) {
                            First = PREDD(PREDD(t1));
                            printff("First forward:    ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            printff("\nFirst backward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            First = PREDD(PREDD(PREDD(N)));
                            printff("\nSecond forward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            printff("\nSecond backward:  ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                        } else {
                            First = SUCC(SUCC(t1));
                            printff("First forward:    ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            printff("\nFirst backward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            First = SUCC(SUCC(SUCC(N)));
                            printff("\nSecond forward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            printff("\nSecond backward:  ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                        } */

            Make3OptMove(t1, t2, t3, t4, t5, t6, 1);
            /*
                        printff("\nAfter:\n");
                        if (Forward) {
                            First = PREDD(PREDD(t1));
                            printff("First forward:    ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            printff("\nFirst backward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            First = PREDD(PREDD(PREDD(N)));
                            printff("\nSecond forward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            printff("\nSecond backward:  ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                        } else {
                            First = SUCC(SUCC(t1));
                            printff("First forward:    ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            printff("\nFirst backward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                            First = SUCC(SUCC(SUCC(N)));
                            printff("\nSecond forward:   ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = PREDD(First);
                            }
                            printff("\nSecond backward:  ");
                            for (int i = 0; i < 8; ++i) {
                                printff("%d-->", First->Id);
                                First = SUCC(First);
                            }
                        } */

            // printff("\nCurrentPenalty %lld\n", CurrentPenalty);
            if (Improvement(&Gain, t1, t2)) {
                // printff("NewPenalty %lld\n", CurrentPenalty - PenaltyGain);
                Swaps = 0;
                return 1;
            } else {
                // printff("**NewPenalty %lld\n", CurrentPenalty - PenaltyGain);
                abort();  // Should never happen
                return 0;
            }
        }
    } while (N != StartRoute);
    return 0;
}
