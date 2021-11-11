#include <math.h>

#include "LKH.h"
/*
 * Solomon, Marius M. (1987).
 * Algorithms for the Vehicle Routing and Scheduling Problems with Time Window Constraints.
 * Operations Research, 35(2), 254–265. doi:10.1287/opre.35.2.254
 *
 * This code follows the paper naming convention.
 *
 * Differently from what suggested in the paper regarding the use of Lemma 1.1,
 * a O(1) test is applied to check for insertion feasibility. At every new
 * insertion the "latest" time of each node is tightened in accordance to the
 * successive nodes of the route, such that, if the O(1) test b(u) < l(u)) is true, then the
 * insertion is known to be feasible without the need of propagating the delay.
 *
 * Another change from what described in the paper regards the "sequential" vs. "parallel" version.
 * The original algorithm is designed as "sequential" constructive algorithm, i.e., routes
 * are built one after the other. While, in this implementation one can choose whether many routes are
 * created at the same time, such that every new customer can be inserted in the best
 * route available at that time (an mitigate the common effect in "sequential" constructives
 * where the last route are of very poor quality).
 */

#define MINIMIZE_ROUTES /* Keep solution with lowest number of routes (default: minimize distance) */

#define TWIN(N) ((N) + DimensionSaved)                   /* Get the "twin" node in the TSP->ATSP transformation */
#define Dist(N1, N2) (OldDistance(N1, TWIN(N2)))         /* Distance shorthand for Nodes */
#define dist(n1, n2) (Dist((n1)->N, (n2)->N))            /* Distance shorthand for CVRPTWNodes */
#define Link2(N1, N2) (((N1)->Next = (N2))->Prev = (N1)) /* Link Nodes using Next and Prev fields */
#define link(n1, n2) (((n1)->j = (n2))->i = (n1))        /* Link cvrptw nodes n1 and n2 together */
#define b(u) ((u)->N->prevCostSum)                       /* Begin timestamp of the service of customer u */
#define e(u) ((u)->N->Earliest)                          /* Earliest time for the start of the service of u */
#define l(u) ((u)->N->prevPenalty)                       /* Laterst feasible time for the start of the service of u. */
#define s(u) ((u)->N->ServiceTime)                       /* Service time of u */
#define dad(n) ((n)->N->Dad)                             /* Representative depot Node at the start of the route of u */
#define cvrptw_n(N) (nodes + (N)->Id - 1)                /* Get cvrptw node of Node* N */
#define Id(n) ((n)->N->Id)                               /* Shorthand to get cvrptw node Id */
#define depId(n) ((n)->N->DepotId)                       /* Shorthand to get cvrptwn node DepotId */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct CVRPTWNode_ {
    Node* N;               /* Corresponding Node* */
    double c_1;            /* Holds c_1 value for current i and j */
    double c_2;            /* Holds c_2 value for current i and j */
    struct CVRPTWNode_* i; /* Predecessor or candidate predecessor */
    struct CVRPTWNode_* j; /* Successor or candidate successor */
    int d_iu;              /* Distance from i */
    int d_uj;              /* Distance from j */
    int d_du;              /* Distance from depot */
    int feasible;          /* True if it is feasible */
} CVRPTWNode;

typedef CVRPTWNode* (*InitStrat)();

typedef struct Insertion1MetaData_ {
    double mu, lambda, alpha1, alpha2;
    InitStrat InitNode;
    int parallel;
} Insertion1MetaData;

static void SolomonI1(GainType* Cost, int* Vehicles);
static Node* next_depot(CVRPTWNode* dep);
static void try_improve_c1(CVRPTWNode* i, CVRPTWNode* u, CVRPTWNode* j);
static void reset_node(CVRPTWNode* u);
static int needs_update(CVRPTWNode* u);
static void setup();
static CVRPTWNode* new_route(CVRPTWNode** first_depot);
static CVRPTWNode* insert_node(CVRPTWNode* u);
static CVRPTWNode* FarthestNode0();
static CVRPTWNode* FarthestNode1();
static CVRPTWNode* FarthestNode2();
static CVRPTWNode* UrgentNode();
static GainType buid_tour();
static int coordinates_are_used();

static Insertion1MetaData I1Data;
static int CurrentVehicles = INT_MAX;
static CVRPTWNode* nodes;

GainType CVRPTW_InitialTour() {
    double EntryTime = GetTime();
    GainType BestCost = PLUS_INFINITY, BestPenalty = PLUS_INFINITY;
    InitStrat FarthestNode = coordinates_are_used() ? FarthestNode1 : FarthestNode2;
    nodes = (CVRPTWNode*)malloc(DimensionSaved * sizeof(CVRPTWNode));

    /* Parameters suggested in the Solomon's paper: mu, lambda, alpha1, alpha2. + First-route-node strategy and sequential/parallel
     * Data obtained with the 56 100-customers Solomon's instances.*/
    Insertion1MetaData I1Conf[] = {
        {1.0, 1.0, 1.0, 0.0, NULL, 0},       /* Best salesmen 19/56. Best length 32/56 */
        {1.0, 2.0, 1.0, 0.0, NULL, 0},       /* Best salesmen 35/56. Best length 24/56 */
        /* {1.0, 1.0, 0.0, 1.0, NULL, 0}, */ /* Best salesmen  1/56. Best length  0/56 <-- remove */
        /* {1.0, 2.0, 0.0, 1.0, NULL, 0}  */ /* Best salesmen  1/56. Best length  0/56 <-- remove */
    };

    for (int i = 0; i < sizeof(I1Conf) / sizeof(*I1Conf); ++i) {
        I1Data = I1Conf[i];
        for (int paral = 0; paral < 2; ++paral) {
            /* sequential (0): Best salesmen 28/56. Best length 14/56 */
            /* parallel   (1): Best salesmen 28/56. Best length 42/56 */
            I1Data.parallel = paral;
            for (int init_strat = 0; init_strat < 3; ++init_strat) {
                /* FarthestNode0 (0): Best salesmen 11/56. Best length 17/56 */
                /* FarthestNode1 (1): Best salesmen 19/56. Best length 18/56 */
                /* UrgentNode    (2): Best salesmen 19/56. Best length 13/56 */
                /* FarthestNode2 (3): Best salesmen  7/56. Best length  8/56 <-- remove */
                switch (init_strat) {
                case 0:
                    I1Data.InitNode = FarthestNode0;
                    break;
                case 1:
                    I1Data.InitNode = FarthestNode;
                    break;
                case 2:
                    I1Data.InitNode = UrgentNode;
                    break;
                    /* case 3:
                        I1Data.InitNode = FarthestNode2;
                        break; */
                }

        int Vehicles = Salesmen;
        GainType Cost = PLUS_INFINITY;
        SolomonI1(&Cost, &Vehicles);
        CurrentPenalty = Penalty();
#ifdef MINIMIZE_ROUTES
        if (Vehicles < CurrentVehicles ||
            (Vehicles == CurrentVehicles && (CurrentPenalty < BestPenalty || (CurrentPenalty == BestPenalty && Cost < BestCost)))) {
#else
        if (CurrentPenalty < BestPenalty || (CurrentPenalty == BestPenalty && Cost < BestCost)) {
#endif
            CurrentVehicles = Vehicles;
            BestPenalty = CurrentPenalty;
            BestCost = Cost;
            for (int i = 1; i <= Dimension; ++i) {
                NodeSet[i].Suc = NodeSet[i].Next;
                NodeSet[i].Pred = NodeSet[i].Prev;
                    }
                }
            }
        }
    }

    if (TraceLevel >= 1) {
        printff("CVRPTW = ");
        if (Salesmen > 1)
            printff(GainFormat "_" GainFormat ", Vehicles = %d", CurrentPenalty, BestCost, CurrentVehicles);
        else
            printff(GainFormat, BestCost);
        if (Optimum != MINUS_INFINITY && Optimum != 0)
            printff(", Gap = %0.2f%%", 100.0 * (BestCost - Optimum) / Optimum);
        printff(", Time = %0.2f sec.\n", fabs(GetTime() - EntryTime));
    }
    free(nodes);
    return BestCost;
}

void SolomonI1(GainType* Cost, int* Vehicles) {
    double EntryTime = GetTime();
    int check_demand = 1;
    CVRPTWNode* first_depot = &nodes[Depot->Id - 1];
    FirstNode = NodeSet + 2;
    int routes = 1;

    setup();

    CVRPTWNode* prev_u = new_route(&first_depot);
    for (int NodesLeft = Dim - 2; NodesLeft; --NodesLeft) {
        /* Note: Next and Prev fields form a 1-tree where the
         * loop is made by nodes for which N->V == 0.
         * The loop can be reached following Next. */
        while (FirstNode->V)
            FirstNode = FirstNode->Next;
        Node* N = FirstNode;
        CVRPTWNode* best_u = cvrptw_n(FirstNode);
        int feasible = I1Data.parallel;
        do {
            CVRPTWNode* u = cvrptw_n(N);
            if (u->i && dad(u->i) != dad(prev_u)) {
                /* Case 1: a different (or new) route has been modified*/
                if (!check_demand || (dad(prev_u)->prevDemandSum + u->N->Demand <= Capacity)) {
                    try_improve_c1(prev_u->i, u, prev_u);
                    try_improve_c1(prev_u, u, prev_u->j);
                }
            } else if (needs_update(u)) {
                /* Case 2: The potential insertion route has changed,full update is needed*/
                reset_node(u);
                CVRPTWNode* u2 = nodes;
                do {
                    if (check_demand && (dad(u2)->prevDemandSum + u->N->Demand > Capacity)) {
                        u2 = cvrptw_n(next_depot(u2));
                        continue;
                    }
                    do
                        try_improve_c1(u2, u, u2->j);
                    while (depId(u2 = u2->j) == 0);
                } while (u2 != first_depot && !(depId(u2) && depId(u2->j)));
                assert(check_demand == 1 || u->i && u->j);
            }
            feasible = I1Data.parallel ? feasible && u->feasible : feasible || u->feasible;
            u->c_2 = I1Data.lambda * u->d_du - u->c_1;
            if ((u->feasible && !best_u->feasible) || (u->feasible == best_u->feasible && u->c_2 > best_u->c_2))
                best_u = u;
        } while ((N = N->Next) != FirstNode);
        if (!feasible && routes < Salesmen) {
            prev_u = new_route(&first_depot);
            ++routes;
        } else if (!feasible && check_demand)
            check_demand = 0;
        else
            prev_u = insert_node(best_u);
    }
    *Vehicles = routes;
    *Cost = buid_tour();
    if (TraceLevel >= 1)
        printff("SolomonI1 (%.0f, %.0f, %.0f, %.0f, %s, %d) = " GainFormat ", Vehicles = %d, Time = %.1f sec.\n", I1Data.mu, I1Data.lambda,
                I1Data.alpha1, I1Data.alpha2,
                (I1Data.InitNode == FarthestNode0   ? "F0"
                 : I1Data.InitNode == FarthestNode1 ? "F1"
                 : I1Data.InitNode == FarthestNode2 ? "F2"
                                                    : "Ur"),
                I1Data.parallel, *Cost, *Vehicles, fabs(GetTime() - EntryTime));
}


void setup() {
    for (int i = 1; i <= DimensionSaved; ++i) {
        Node* N = &NodeSet[i];
        N->V = 0;
        CVRPTWNode* n = cvrptw_n(N);
        n->N = N;
        n->d_iu = n->d_uj = n->d_du = N->DepotId ? 0 : Dist(Depot, N);
        l(n) = N->Latest;
        reset_node(n);
    }
    /* Create a loop with all the customers */
    int Customers = Dim - 1;
    for (int i = 0; i < Customers; ++i)
        Link2(NodeSet + i + 2, NodeSet + ((i + 1) % Customers) + 2);
    /* Create another loop with all the depots */
    link(nodes, nodes + Dim);
    Depot->Dad = Depot;
    for (int i = Dim; i < DimensionSaved; ++i) {
        link(nodes + i, nodes + ((i + 1) % DimensionSaved));
        Node* N = NodeSet + i + 1;
        N->Dad = N;
        N->prevDemandSum = 0;
    }
}

void reset_node(CVRPTWNode* u) {
    u->i = u->j = NULL;
    u->c_1 = 10e20;
    u->c_2 = -10e20;
    u->feasible = 0;
    b(u) = 0;
}

int needs_update(CVRPTWNode* u) {
    if (!u->i || !u->j)
        return 1;
    if (u->i->j != u->j)
        return 1;
    GainType b_u = MAX(b(u->i) + s(u->i) + u->d_iu, e(u));
    return b(u) != b_u;
}


void try_improve_c1(CVRPTWNode* i, CVRPTWNode* u, CVRPTWNode* j) {
    CVRPTWNode cand_u_ = *u;
    CVRPTWNode* cand_u = &cand_u_;
    cand_u->i = i;
    cand_u->j = j;
    cand_u->d_iu = depId(i) ? cand_u->d_du : dist(i, u);
    cand_u->d_uj = depId(j) ? cand_u->d_du : dist(u, j);

    double b_cand_u = MAX(b(i) + s(i) + cand_u->d_iu, e(u));
    double b_j_u = MAX(b_cand_u + s(cand_u) + cand_u->d_uj, e(j));
    double c_11 = cand_u->d_iu + cand_u->d_uj - I1Data.mu * i->d_uj;
    double c_12 = b_j_u - b(j);
    cand_u->c_1 = I1Data.alpha1 * c_11 + I1Data.alpha2 * c_12;

    double l_cand_u = MIN(l(j) - s(cand_u) - cand_u->d_uj, l(cand_u));
    cand_u->feasible = b_cand_u <= l_cand_u;
    if ((cand_u->feasible && !u->feasible) || (cand_u->feasible == u->feasible && cand_u->c_1 < u->c_1))
        *u = *cand_u;
}

Node* next_depot(CVRPTWNode* n) {
    int DId = dad(n)->DepotId;
    return (DId < Salesmen ? (NodeSet + Dim + DId) : Depot);
}

CVRPTWNode* new_route(CVRPTWNode** first_depot) {
    CVRPTWNode* u = I1Data.InitNode();
    link(u, (*first_depot)->j);
    link(*first_depot, u);
    *first_depot = u->j;
    u->d_iu = u->d_uj = u->i->d_uj = u->j->d_iu = u->d_du;
    b(u) = MAX(b(u->i) + s(u->i) + u->d_iu, e(u));
    l(u) = MIN(l(u->j) - s(u) - u->d_uj, l(u));
    (dad(u) = dad(u->i))->prevDemandSum = u->N->Demand;
    Node* SeedN = u->N;
    SeedN->V = 1;
    Link2(SeedN->Prev, SeedN->Next);
    return u;
}

CVRPTWNode* insert_node(CVRPTWNode* u) {
    assert(u->i->j == u->j && u->j->i == u->i);
    u->i->j = u->j->i = u;
    u->i->d_uj = u->d_iu;
    u->j->d_iu = u->d_uj;
    (dad(u) = dad(u->i))->prevDemandSum += u->N->Demand;
    Node* N = u->N;
    N->V = 1;
    Link2(N->Prev, N->Next);
    CVRPTWNode* orig_u = u;
    b(u) = MAX(b(u->i) + s(u->i) + u->d_iu, e(u));
    while (depId(u = u->j) == 0) {
        assert(u->N->V == 1);
        b(u) = b(u->i) + s(u->i) + u->d_iu;
        if (b(u) <= e(u)) {
            b(u) = e(u);
            break;
        }
    }
    u = orig_u;
    do
        l(u) = MIN(l(u->j) - s(u) - u->d_uj, l(u));
    while (depId(u = u->i) == 0);
    return orig_u;
}

/* Return farthest node from depot. */
CVRPTWNode* FarthestNode0() {
    while (FirstNode->V)
        FirstNode = FirstNode->Next;
    Node* N = FirstNode;
    CVRPTWNode* farthest_n = cvrptw_n(N);
    do {
        CVRPTWNode* n = cvrptw_n(N);
        if (n->d_du > farthest_n->d_du)
            farthest_n = n;
    } while ((N = N->Next) != FirstNode);
    return farthest_n;
}

/* Return farthest node from the barycenter of the nodes
 * already inserted.*/
CVRPTWNode* FarthestNode1() {
    GainType X = Depot->X, Y = Depot->Y, Z = Depot->Z;
    int size = 1;
    CVRPTWNode* n = cvrptw_n(Depot);
    Node* N;
    do {
        N = n->N;
        if (N->DepotId == 0) {
            X += N->X;
            Y += N->Y;
            Z += N->Z;
            ++size;
        }
    } while ((n = n->j) != cvrptw_n(Depot));
    Node Barycenter;
    Barycenter.X = X / size;
    Barycenter.Y = Y / size;
    Barycenter.Z = Z / size;
    while (FirstNode->V)
        FirstNode = FirstNode->Next;
    int MaxDist = 0;
    Node* FarthestN = N = FirstNode;
    do {
        if (N->V)
            continue;
        int distance = OriginalDistance(N, &Barycenter);
        if (distance > MaxDist) {
            MaxDist = distance;
            FarthestN = N;
        }
    } while ((N = N->Next) != FirstNode);
    return cvrptw_n(FarthestN);
}

/* Return the node that maximized the distance with all the
 * inserted nodes.*/
CVRPTWNode* FarthestNode2() {
    while (FirstNode->V)
        FirstNode = FirstNode->Next;
    GainType MaxDistSum = 0;
    Node* N = FirstNode;
    Node* FarthestN = N;
    do {
        if (N->V)
            continue;
        assert(N->DepotId == 0);
        GainType DistSum = Dist(Depot, N);
        CVRPTWNode* n = cvrptw_n(Depot);
        do {
            if (depId(n) == 0)
                DistSum += Dist(n->N, N);
        } while ((n = n->j) != cvrptw_n(Depot));
        if (DistSum > MaxDistSum) {
            MaxDistSum = DistSum;
            FarthestN = N;
        }
    } while ((N = N->Next) != FirstNode);
    return cvrptw_n(FarthestN);
}

CVRPTWNode* UrgentNode() {
    while (FirstNode->V)
        FirstNode = FirstNode->Next;
    Node* N = FirstNode;
    CVRPTWNode* urgent_n = cvrptw_n(N);
    do {
        CVRPTWNode* n = cvrptw_n(N);
        if (l(urgent_n) - urgent_n->d_du > l(n) - n->d_du)
            urgent_n = n;
    } while ((N = N->Next) != FirstNode);
    return urgent_n;
}

GainType buid_tour() {
    GainType Cost = 0;
    CVRPTWNode* depot = cvrptw_n(Depot);
    CVRPTWNode* n = depot;
    do {
        Node* N = n->N;
        Node* NextN = n->j->N;
        assert(depId(n) || b(n) == MAX(b(n->i) + s(n->i) + n->d_iu, e(n)));
        Link2(TWIN(N), N);
        Link2(N, TWIN(NextN));
        Cost += n->d_uj;
    } while ((n = n->j) != depot);
    return Cost;
}

int coordinates_are_used() {
    return OriginalDistance == Distance_ATT || OriginalDistance == Distance_CEIL_2D || OriginalDistance == Distance_CEIL_3D ||
           OriginalDistance == Distance_EUC_2D || OriginalDistance == Distance_EUC_3D || OriginalDistance == Distance_FLOOR_2D ||
           OriginalDistance == Distance_FLOOR_3D || OriginalDistance == Distance_GEO || OriginalDistance == Distance_GEOM ||
           OriginalDistance == Distance_MAN_2D || OriginalDistance == Distance_MAN_3D || OriginalDistance == Distance_MAX_2D ||
           OriginalDistance == Distance_MAX_3D || OriginalDistance == Distance_GEO_MEEUS || OriginalDistance == Distance_GEOM_MEEUS ||
           OriginalDistance == Distance_TOR_2D || OriginalDistance == Distance_TOR_3D || OriginalDistance == Distance_XRAY1 ||
           OriginalDistance == Distance_XRAY2;
}