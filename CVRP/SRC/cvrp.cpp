#include "extract_routes.hpp"

extern "C" {
#include "LKH.h"
#include "Segment.h"
};


class CVRPChecker {
public:
    void add_node(Node *N) {
        current_demand += N->Demand;
        dist = OldDistance(prev, N + (DimensionSaved * Asymmetric));
        current_length += dist;
        prev = N;
        ++size;
    };

    void clear_route(Node *Prev) {
        current_demand = current_length = 0;
        size = 0;
        prev = Prev;
    };

    int is_feasible() const { return current_demand <= Capacity && size > 1UL; }
    inline GainType get_length() const { return current_length; };
    inline GainType get_demand() const { return current_demand; };
    inline size_t get_size() const { return size; };
    inline int get_dist() const { return dist; };

private:
    int dist = 0;
    Node *prev = Depot;
    GainType current_demand = 0;
    GainType current_length = 0;
    size_t size = 0;
};

void Extract_routes(GainType Cost) { extract_routes_tmlp<CVRPChecker>(Cost); }

extern "C" {


/**
 * Modified Penalty function for CVRP instances.
 * The basic idea behind this optimization is to exploit the fact
 * that at each call only few routes (aka petals) have been
 * modified, and we can retrieve this routes simply by looking
 * at the saved Opt moves in the SwapStack array.
 * The implementantio becames less general (since not all the
 * checked tour are obtained from Opt moves), so in some
 * special (and luckily few) cases the old penalty is still called.
 *
 * At the beginning of the function a setup phase is needed to compute
 * the previous partial penalty associated to the involved routes.
 *
 * In this phase, each route is considered only once using the "flag" field
 * of the RouteData.
 * Since depot-copies are part of two differen routes, when counting the
 * number of routes they are ignored (to avoid ambiguities).
 * Indeed, depot-copies are not assigned to any real route. They all point
 * to the same "dummy" RouteData (at index 0 or RouteData array).
 *
 * After the setup phase, the actual check starts.
 * As starting point for the penalty computation, only the non-depot nodes
 * of the current r-Opt move in the SwapStack are considered. Each
 * non-depot node of the move is marked using the PFlag.
 * In this way, before starting to iterate over a route we can verify if that
 * routed has been alredy been checked. When a unexplored route is found
 * all the PFlag of its nodes are set to 0 again to avoid multiple checks
 * for the same route.
 *
 * The route iteration proceed from the starting node forward until a
 * depot-copy is found. Then it start again from the starting node
 * and proceed backward, again untile a depot-copy is found.
 *
 * When an improvement is found, the update function is called.
 * This function simply iterates over the full tour, updateding each
 * RouteData value. The index of each route is given by the DepotId of
 * the depot-copy at the start of that route following the current
 * tour orientation (which go from 1 to SALESMEN).
 * In this way, every node of the the tour has its own route associated,
 * with its previous penalty.
 */

GainType Penalty_Old();
static void update_Penalty();
static GainType oldPenaltySum;
static int setup_Node_CVRP(Node *);
static int setup_Penalty();

GainType Penalty() {
    GainType P = 0;
    if (Swaps && cava_PetalsData) {
        GainType DemandSum;
        Node *N;

        // Moves that only touch one route cannot change the penalty value
        if (setup_Penalty() == 1) return CurrentPenalty;

        for (SwapRecord *si = SwapStack + Swaps - 1; si >= SwapStack; --si)
            for (int twice = 0; twice < 2; ++twice) {
                if (twice > 0) N = si->t2->PFlag ? si->t2 : si->t3;
                else
                    N = si->t1->PFlag ? si->t1 : si->t4;

                if (N->PFlag) {
                    DemandSum = N->Demand;
                    Node *savedN = N;
                    N->PFlag = 0;

                    // Forward
                    while ((N = SUC(N))->DepotId == 0) {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                    }

                    GainType tempP = P + DemandSum - Capacity;
                    if (DemandSum > Capacity && (tempP > oldPenaltySum || (tempP == oldPenaltySum && CurrentGain <= 0))) {
                        for (SwapRecord *s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;

                        return CurrentPenalty + (CurrentGain > 0);
                    }

                    // Backward
                    N = savedN;
                    while ((N = PRED(N))->DepotId == 0) {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                    }

                    if (DemandSum > Capacity && ((P += DemandSum - Capacity) > oldPenaltySum || (P == oldPenaltySum && CurrentGain <= 0))) {
                        for (SwapRecord *s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;

                        return CurrentPenalty + (CurrentGain > 0);
                    }
                }
            }

        if (!CurrentPenalty) return P;

        if (P < oldPenaltySum || (P == oldPenaltySum && CurrentGain > 0)) {
            update_Penalty();  // Improved!
            return CurrentPenalty + P - oldPenaltySum;
        } else
            return CurrentPenalty + (CurrentGain > 0);
    } else {
        P = Penalty_Old();
        if (P < CurrentPenalty || (P == CurrentPenalty && CurrentGain > 0)) {
            if (!cava_PetalsData) cava_PetalsData = (RouteData *)calloc(Salesmen + 1, sizeof(RouteData));
            update_Penalty();
        }
        return P;
    }
}

/* Returns 1 if only one route is involved in the current move */
static int setup_Penalty() {
    oldPenaltySum = 0;
    if (CurrentPenalty) {
        int petalCounter = 0;
        for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s) {
            // If a Depot->Depot edge is found, an additional empty petal needs to be counted
            petalCounter += (s->t1->DepotId && s->t2->DepotId) || (s->t3->DepotId && s->t4->DepotId);

            petalCounter += setup_Node_CVRP(s->t1) + setup_Node_CVRP(s->t2) + setup_Node_CVRP(s->t3) + setup_Node_CVRP(s->t4);
        }

        // Reset petals flags for next petal counting
        for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s) {
            s->t1->PetalId->flag = 0;
            s->t2->PetalId->flag = 0;
            s->t3->PetalId->flag = 0;
            s->t4->PetalId->flag = 0;
        }

        if (petalCounter == 1) return 1;  // early return
    }

    for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s) {
        s->t1->PFlag = !s->t1->DepotId;
        s->t2->PFlag = !s->t2->DepotId;
        s->t3->PFlag = !s->t3->DepotId;
        s->t4->PFlag = !s->t4->DepotId;
    }

    return 0;
}

static int setup_Node_CVRP(Node *N) {
    if (!N->PetalId->flag) {
        oldPenaltySum += N->PetalId->OldPenalty;
        N->PetalId->flag = 1;
        return (N->PetalId != cava_PetalsData);  // Depots have PetalId_index == 0
    }

    return 0;
}

/* Update route data when a new improving tour is found */
static void update_Penalty() {
    Node *N = Depot;
    RouteData *CurrId;
    GainType DemandSum;
    do {
        DemandSum = 0;
        N->PetalId = cava_PetalsData;  // depots point to 0 cell
        CurrId = cava_PetalsData + N->DepotId;
        while ((N = SUCC(N))->DepotId == 0) {
            N->PetalId = CurrId;
            DemandSum += N->Demand;
        }
        CurrId->OldPenalty = DemandSum > Capacity ? DemandSum - Capacity : 0;
    } while (N != Depot);
}

GainType Penalty_Old() {
    static Node *StartRoute = 0;
    Node *N, *CurrentRoute;
    GainType DemandSum, DistanceSum, P = 0;

    if (!StartRoute) StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved) StartRoute -= DimensionSaved;
    N = StartRoute;
    do {
        CurrentRoute = N;
        DemandSum = 0;
        do
            DemandSum += N->Demand;
        while ((N = SUCC(N))->DepotId == 0);
        if (DemandSum > Capacity && ((P += DemandSum - Capacity) > CurrentPenalty || (P == CurrentPenalty && CurrentGain <= 0))) {
            StartRoute = CurrentRoute;
            return CurrentPenalty + (CurrentGain > 0);
        }
        if (DistanceLimit != DBL_MAX) {
            DistanceSum = 0;
            N = CurrentRoute;
            do {
                DistanceSum += (C(N, SUCC(N)) - N->Pi - SUCC(N)->Pi) / Precision;
                if (!N->DepotId) DistanceSum += N->ServiceTime;
            } while ((N = SUCC(N))->DepotId == 0);
            if (DistanceSum > DistanceLimit && ((P += DistanceSum - DistanceLimit) > CurrentPenalty || (P == CurrentPenalty && CurrentGain <= 0))) {
                StartRoute = CurrentRoute;
                return CurrentPenalty + (CurrentGain > 0);
            }
        }
    } while (N != StartRoute);
    return P;
}


/*
 * The Forbidden function is used to test if an edge, (Na,Nb),
 * is not allowed to belong to a tour.
 * If the edge is forbidden, the function returns 1; otherwise 0.
 */
int Forbidden(Node *Na, Node *Nb) {
    if (Na == Nb) return 1;
    if (InInitialTour(Na, Nb) || Na->Id == 0 || Nb->Id == 0) return 0;
    if (Na->Head && !FixedOrCommon(Na, Nb) && (Na->Head == Nb->Head || (Na->Head != Na && Na->Tail != Na) || (Nb->Head != Nb && Nb->Tail != Nb))) return 1;
    if (Salesmen > 1 && Dimension == DimensionSaved) {
        if (Na->DepotId) {
            if (Nb->DepotId || (Nb->Special && Nb->Special != Na->DepotId && Nb->Special != Na->DepotId % Salesmen + 1)) return 1;
        }
        if (Nb->DepotId) {
            if (Na->DepotId || (Na->Special && Na->Special != Nb->DepotId && Na->Special != Nb->DepotId % Salesmen + 1)) return 1;
        }
    }
    return 0;
}
}