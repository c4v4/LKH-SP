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
 * to check for insertion feasibility a O(1) test is applied. At every new
 * insertion the "latest" time of each node is tightened in accordance to the
 * successive nodes, such that, if the O(1) test b(u) < l(u)) is true then the
 * insertion is feasible for that route.
 */

#define TWIN(N) ((N) + DimensionSaved)            /* Get the "twin" node in the TSP->ATSP transformation */
#define Dist(N1, N2) (OldDistance(N1, TWIN(N2)))  /* Distance shorthand for Nodes */
#define dist(n1, n2) (Dist((n1)->N, (n2)->N))     /* Distance shorthand for CVRPTWNodes */
#define b(u) ((u)->N->prevCostSum)                /* Begin timestamp of the service of customer u */
#define e(u) ((u)->N->Earliest)                   /* Earliest time for the start of the service of u */
#define l(u) ((u)->N->prevPenalty)                /* Laterst feasible time for the start of the service of u. */
#define s(u) ((u)->N->ServiceTime)                /* Service time of u */
#define dad(n) ((n)->N->Dad)                      /* Representative depot Node at the start of the route of u */
#define cvrptw_n(N) (nodes + (N)->Id - 1)         /* Get cvrptw node of Node* N */
#define link(n1, n2) (((n1)->j = (n2))->i = (n1)) /* Link cvrptw nodes n1 and n2 together */
#define Id(n) ((n)->N->Id)                        /* Shorthand to get cvrptw node Id */
#define depId(n) ((n)->N->DepotId)                /* Shorthand to get cvrptwn node DepotId */

typedef struct CVRPTWNode_ {
    Node* N;               /* Corresponding Node* */
    double c;              /* Holds c_1 value in the internal loop, then it holds c_2. */
    struct CVRPTWNode_* i; /* Predecessor or candidate predecessor */
    struct CVRPTWNode_* j; /* Successor or candidate successor */
    int d_iu;              /* Distance from i */
    int d_uj;              /* Distance from j */
    int d_du;              /* Distance from depot */
    int time_feas;         /* True if it is time-feasible */
} CVRPTWNode;

typedef CVRPTWNode* (*InitStrat)(CVRPTWNode*);

static struct Insertion1MetaData {
    double mu, lambda, alpha1, alpha2;
    InitStrat InitNode;
} I1Data;

static int CurrentRoutes = INT_MAX;

static void SolomonI1(CVRPTWNode* nodes);
static Node* next_depot(CVRPTWNode* dep);
static void check_improve(CVRPTWNode* i, CVRPTWNode* u, CVRPTWNode* j);
static void reset_node(CVRPTWNode* u);
static CVRPTWNode* FarthestNode1(CVRPTWNode* nodes);
static CVRPTWNode* FarthestNode2(CVRPTWNode* nodes);
static CVRPTWNode* UrgentNode(CVRPTWNode* nodes);
static void setup(CVRPTWNode* nodes);
static CVRPTWNode* new_route(CVRPTWNode* nodes, CVRPTWNode** first_depot);
static CVRPTWNode* insert_node(CVRPTWNode* u);
static GainType buid_tour(CVRPTWNode* nodes);

static double max(double a, double b) { return a > b ? a : b; }
static double min(double a, double b) { return a < b ? a : b; }
static double c_2(CVRPTWNode* u) { return I1Data.lambda * u->d_du - u->c; }

static int counter = 0;
static int winner = 0;
GainType CVRPTW_InitialTour2() {
    double EntryTime = GetTime();
    printff("CVRPTW = ");
    CVRPTWNode* nodes = (CVRPTWNode*)malloc(DimensionSaved * sizeof(CVRPTWNode));
    InitStrat FarthestNode;
    if (OriginalDistance == Distance_ATT || OriginalDistance == Distance_CEIL_2D || OriginalDistance == Distance_CEIL_3D ||
        OriginalDistance == Distance_EUC_2D || OriginalDistance == Distance_EUC_3D || OriginalDistance == Distance_FLOOR_2D ||
        OriginalDistance == Distance_FLOOR_3D || OriginalDistance == Distance_GEO || OriginalDistance == Distance_GEOM ||
        OriginalDistance == Distance_MAN_2D || OriginalDistance == Distance_MAN_3D || OriginalDistance == Distance_MAX_2D ||
        OriginalDistance == Distance_MAX_3D || OriginalDistance == Distance_GEO_MEEUS || OriginalDistance == Distance_GEOM_MEEUS ||
        OriginalDistance == Distance_TOR_2D || OriginalDistance == Distance_TOR_3D || OriginalDistance == Distance_XRAY1 ||
        OriginalDistance == Distance_XRAY2)
        FarthestNode = FarthestNode1;
    else
        FarthestNode = FarthestNode2;

    /* Parameters suggested in the original paper */
    I1Data.mu = 1;
    I1Data.lambda = 1;
    I1Data.alpha1 = 1;
    I1Data.alpha2 = 0;
    I1Data.InitNode = FarthestNode;
    SolomonI1(nodes);
    I1Data.InitNode = UrgentNode;
    SolomonI1(nodes);

    I1Data.lambda = 2;
    I1Data.alpha1 = 1;
    I1Data.alpha2 = 0;
    I1Data.InitNode = FarthestNode;
    SolomonI1(nodes);
    I1Data.InitNode = UrgentNode;
    SolomonI1(nodes);

    I1Data.lambda = 1;
    I1Data.alpha1 = 0;
    I1Data.alpha2 = 1;
    I1Data.InitNode = FarthestNode;
    SolomonI1(nodes);
    I1Data.InitNode = UrgentNode;
    SolomonI1(nodes);

    I1Data.lambda = 2;
    I1Data.alpha1 = 0;
    I1Data.alpha2 = 1;
    I1Data.InitNode = FarthestNode;
    SolomonI1(nodes);
    I1Data.InitNode = UrgentNode;
    SolomonI1(nodes);

    for (int i = 1; i <= Dimension; ++i) {
        NodeSet[i].Suc = NodeSet[i].Next;
        NodeSet[i].Pred = NodeSet[i].Prev;
    }
    if (TraceLevel >= 1) {
        if (Salesmen > 1)
            printff(GainFormat "_" GainFormat, CurrentPenalty, BestCost);
        else
            printff(GainFormat, BestCost);
        if (Optimum != MINUS_INFINITY && Optimum != 0)
            printff(", Gap = %0.2f%%", 100.0 * (BestCost - Optimum) / Optimum);
        printff(", Time = %0.2f sec.\n", fabs(GetTime() - EntryTime));
        printff("Winner: %d | ", winner);
    }
    free(nodes);
    return BestCost;
}


void SolomonI1(CVRPTWNode* nodes) {
    ++counter;
    double EntryTime = GetTime();
    setup(nodes);

    int check_demand = 1;
    FirstNode = NodeSet + 2;
    CVRPTWNode* first_depot = &nodes[Depot->Id - 1];
    CVRPTWNode* prev_u = new_route(nodes, &first_depot);
    int routes = 1;
    for (int NodesLeft = Dim - 2; NodesLeft; --NodesLeft) {

        /* Note: Suc and Pred fields form a 1-tree where the
         * loop is made by nodes for which N->V == 0.
         * The loop can be reached following Suc. */
        while (FirstNode->V)
            FirstNode = FirstNode->Suc;

        Node* N = FirstNode;
        CVRPTWNode* best_u = cvrptw_n(FirstNode);
        int feasible = 1;
        do {
            CVRPTWNode* u = cvrptw_n(N);
            assert(N->V == 0);
            reset_node(u);

            CVRPTWNode* u2 = nodes;
            do {
                if (check_demand && (dad(u2)->prevDemandSum + u->N->Demand > Capacity)) {
                    u2 = cvrptw_n(next_depot(u2));
                    continue;
                }
                do
                    check_improve(u2, u, u2->j);
                while (depId(u2 = u2->j) == 0);
            } while (u2 != first_depot && !(depId(u2) && depId(u2->j)));
            assert(u->i && u->j);
            feasible &= u->time_feas;
            u->c = c_2(u);
            if ((u->time_feas && !best_u->time_feas) || (u->time_feas == best_u->time_feas && u->c > best_u->c))
                best_u = u;
        } while ((N = N->Suc) != FirstNode);

        if (!feasible && routes < Salesmen) {
            prev_u = new_route(nodes, &first_depot);
            ++routes;
        } else if (!feasible && check_demand)
            check_demand = 0;
        else
            prev_u = insert_node(best_u);
    }

    GainType CurrentCost = buid_tour(nodes);
    CurrentPenalty = Penalty();
    // if (CurrentPenalty < BestPenalty || (CurrentPenalty == BestPenalty && CurrentCost < BestCost)) {
    if (routes < CurrentRoutes) {
        CurrentRoutes = routes;
        BestPenalty = CurrentPenalty;
        BestCost = CurrentCost;
        for (int i = 1; i <= Dimension; ++i) {
            NodeSet[i].Next = NodeSet[i].Suc;
            NodeSet[i].Prev = NodeSet[i].Pred;
        }
        if (TraceLevel >= 2)
            printff("*");
        winner = counter;
    }
    if (TraceLevel >= 2)
        printff("CVRPTW (%.0f, %.0f, %.0f, %.0f) = " GainFormat "_" GainFormat ", Vehicles = %d, Time = %.1f sec.\n", I1Data.mu,
                I1Data.lambda, I1Data.alpha1, I1Data.alpha2, CurrentPenalty, CurrentCost, routes, fabs(GetTime() - EntryTime));
}


void setup(CVRPTWNode* nodes) {
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
        Link(NodeSet + i + 2, NodeSet + ((i + 1) % Customers) + 2);
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
    u->c = 10e20;
    u->time_feas = 0;
    b(u) = 0;
    return;
}

void check_improve(CVRPTWNode* i, CVRPTWNode* u, CVRPTWNode* j) {
    CVRPTWNode cand_u_ = *u;
    CVRPTWNode* cand_u = &cand_u_;
    cand_u->i = i;
    cand_u->j = j;
    cand_u->d_iu = depId(i) ? cand_u->d_du : dist(i, u);
    cand_u->d_uj = depId(j) ? cand_u->d_du : dist(u, j);

    double b_cand_u = max(b(i) + s(i) + cand_u->d_iu, e(u));
    double b_j_u = max(b_cand_u + s(cand_u) + cand_u->d_uj, e(j));
    double c_11 = cand_u->d_iu + cand_u->d_uj - I1Data.mu * i->d_uj;
    double c_12 = b_j_u - b(j);
    cand_u->c = I1Data.alpha1 * c_11 + I1Data.alpha2 * c_12;

    double maxb_cand_u = min(l(j) - s(cand_u) - cand_u->d_uj, l(cand_u));
    cand_u->time_feas = b_cand_u <= maxb_cand_u;  // is_time_feasible(cand_u);
    if ((cand_u->time_feas && !u->time_feas) || (cand_u->time_feas == u->time_feas && cand_u->c < u->c))
        *u = *cand_u;
}

Node* next_depot(CVRPTWNode* n) {
    int DId = dad(n)->DepotId;
    assert(DId);
    Node* N = (DId < Salesmen ? (NodeSet + Dim + DId) : Depot);
    return N;
}

CVRPTWNode* new_route(CVRPTWNode* nodes, CVRPTWNode** first_depot) {
    CVRPTWNode* seed_n = I1Data.InitNode(nodes);
    link(seed_n, (*first_depot)->j);
    link(*first_depot, seed_n);
    seed_n->i->d_uj = seed_n->d_iu = seed_n->d_uj = seed_n->j->d_iu = seed_n->d_du;
    b(seed_n) = max(b(seed_n->i) + s(seed_n->i) + seed_n->d_iu, e(seed_n));
    l(seed_n) = min(l(seed_n->j) - s(seed_n) - seed_n->d_uj, l(seed_n));
    (dad(seed_n) = (*first_depot)->N)->prevDemandSum = seed_n->N->Demand;
    *first_depot = seed_n->j;
    Node* SeedN = seed_n->N;
    assert(SeedN->V == 0);
    SeedN->V = 1;
    Link(SeedN->Pred, SeedN->Suc);
    return seed_n;
}

CVRPTWNode* insert_node(CVRPTWNode* u) {
    assert(u->i->j == u->j && u->j->i == u->i);
    u->i->j = u;
    u->j->i = u;
    u->i->d_uj = u->d_iu;
    u->j->d_iu = u->d_uj;
    (dad(u) = dad(u->i))->prevDemandSum += u->N->Demand;
    Node* MinN = u->N;
    assert(MinN->V == 0);
    MinN->V = 1;
    Link(MinN->Pred, MinN->Suc);
    assert(!(u->time_feas && (dad(u)->prevDemandSum > Capacity)));
    CVRPTWNode* orig_u = u;
    b(u) = max(b(u->i) + s(u->i) + u->d_iu, e(u));
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
        l(u) = min(l(u->j) - s(u) - u->d_uj, l(u));
    while (depId(u = u->i) == 0);
    return orig_u;
}

/* Return farthest node from the barycenter of the nodes
 * already inserted.*/
CVRPTWNode* FarthestNode1(CVRPTWNode* nodes) {
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
        FirstNode = FirstNode->Suc;
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
    } while ((N = N->Suc) != FirstNode);
    return cvrptw_n(FarthestN);
}

/* Return the node that maximized the distance with all the
 * inserted nodes.*/
CVRPTWNode* FarthestNode2(CVRPTWNode* nodes) {
    while (FirstNode->V)
        FirstNode = FirstNode->Suc;
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
    } while ((N = N->Suc) != FirstNode);
    return cvrptw_n(FarthestN);
}

CVRPTWNode* UrgentNode(CVRPTWNode* nodes) {
    while (FirstNode->V)
        FirstNode = FirstNode->Suc;
    Node* N = FirstNode;
    CVRPTWNode* urgent_n = cvrptw_n(N);
    do {
        CVRPTWNode* n = cvrptw_n(N);
        if (l(urgent_n) - urgent_n->d_du > l(n) - n->d_du)
            urgent_n = n;
    } while ((N = N->Suc) != FirstNode);
    return urgent_n;
}

GainType buid_tour(CVRPTWNode* nodes) {
    GainType Cost = 0;
    CVRPTWNode* depot = cvrptw_n(Depot);
    CVRPTWNode* n = depot;
    do {
        Node* N = n->N;
        Node* NextN = n->j->N;
        assert(depId(n) || (double)b(n) == max(b(n->i) + s(n->i) + n->d_iu, e(n)));
        Link(TWIN(N), N);
        Link(N, TWIN(NextN));
        Cost += n->d_uj;
    } while ((n = n->j) != depot);
    return Cost;
}