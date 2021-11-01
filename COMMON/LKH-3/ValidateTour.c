#include <string.h>

#include "LKH.h"

/**
 * ValidateTour check if a given tour (given in its contigous array representation) is valid.
 * If it is, it applies the Jonker and Volgenant MTSP-TSP transformation.
 * In the original LKH-3, this transformation is not required for "InitialTour",
 * while it is for MergeTours.
 * However, we can avoid to add back edges that had been removed from the transformation,
 * applying this also for InitialTour (also because it checks quite deeply the input tour)
 *
 */

#undef NDEBUG

#define CALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)calloc(nelem, sizeof(TYPE))))
#define MALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)malloc((nelem) * sizeof(TYPE))))
#define REALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)realloc(ptr, (nelem) * sizeof(TYPE))))

#define FREEN0(a) \
    do {          \
        free(a);  \
        a = NULL; \
    } while (0)

typedef struct petal_tag {
    Node *head;
    Node *tail;
    Node *depHead;
    Node *depTail;
    int taken;
} Petal;

static int DepotCheckForbidden(Node *Na, Node *Nb);
static void reversePetal(Petal *p);

void ValidateTour(int *tour) {
    if (Salesmen > 1) {
        if (ProblemType == CVRP || ProblemType == CTSP || ProblemType == TSP) {
            int index = 0;
            Petal *petals = NULL;
            Node **depots = NULL;
            CALLOC(depots, Salesmen, Node *);
            CALLOC(petals, Salesmen, Petal);

            // Setup
            index = 0;
            int depIndex = 0;
            for (int i = 1; i <= DimensionSaved; ++i) {
                Node *N = NodeSet + i;
                if (N->DepotId) {  // Be sure to find all the depots and set them properly
                    depots[depIndex++] = N;
                    N->Suc = N->Pred = NULL;
                    N->V = 0;
                }

                assert(tour[i] && tour[i - 1]);
                N = NodeSet + tour[i];
                N->V = 0;
                N->Pred = NodeSet + tour[i - 1];
                N->Suc = NodeSet + tour[i + 1 > Dimension ? 1 : i + 1];
                if (N->Pred->DepotId)
                    N->Pred = NULL;
                if (N->Suc->DepotId)
                    N->Suc = NULL;
                if (N->DepotId) {
                    petals[index++].tail = N->Pred;
                    petals[index % Salesmen].head = N->Suc;
                    N->Pred = N->Suc = NULL;  // Set depot potentially again
                }
            }

            // check
            for (int c = 0; c < Salesmen; ++c) {
                assert(petals[c].head);
                assert(petals[c].tail);
                assert(!petals[c].head->Pred);
                assert(!petals[c].tail->Suc);
                assert(depots[c]);
            }

            // Single node petals
            for (int c = 0; c < Salesmen; ++c) {
                if (petals[c].head == petals[c].tail && petals[c].head->Special) {
                    for (int d = Salesmen - 1; d >= 0; --d) {
                        if (depots[d]->DepotId == petals[c].head->Special)
                            (petals[c].depHead = depots[d])->V++;
                        if (depots[d]->DepotId % Salesmen + 1 == petals[c].tail->Special)
                            (petals[c].depTail = depots[d])->V++;
                        if (petals[c].depHead && petals[c].depTail)
                            break;
                    }
                    assert(petals[c].depHead);
                    assert(petals[c].depTail);
                }
            }

            // All the others
            for (int c = 0; c < Salesmen; ++c) {
                int depotToFind = (petals[c].head->Special != 0) + (petals[c].tail->Special != 0);
                if (depotToFind && petals[c].head != petals[c].tail) {
                    for (int d = Salesmen - 1; d >= 0; --d) {
                        if (depots[d]->DepotId == petals[c].head->Special)
                            (petals[c].depHead = depots[d])->V++;
                        else if (depots[d]->DepotId == petals[c].tail->Special)
                            (petals[c].depTail = depots[d])->V++;
                        if ((petals[c].depHead != NULL) + (petals[c].depTail != NULL) == depotToFind)
                            break;
                    }
                    // Check
                    if (petals[c].depHead)
                        assert(petals[c].depHead->DepotId && petals[c].depHead->DepotId == petals[c].head->Special);
                    if (petals[c].depTail)
                        assert(petals[c].depTail->DepotId && petals[c].depTail->DepotId == petals[c].tail->Special);
                }
            }

            int addedPetals = 1;
            // First petal added
            Petal *currentPetal = NULL, *initialPetal;
            // Find a first petal without a initial depot
            for (int c = 0; c < Salesmen; ++c) {
                assert((currentPetal = &petals[c]));
                if (!currentPetal->depHead)
                    break;
                else if (!currentPetal->depTail) {
                    reversePetal(currentPetal);
                    break;
                }
            }
            // If all petals are of size 1 this fails
            assert(!currentPetal->depHead);
            initialPetal = currentPetal;

            currentPetal->taken = 1;
            do {
                if (!currentPetal->depTail) {  // Depot not present and no specific depot required
                    int d;
                    for (d = 0; d < Salesmen; ++d)
                        if (!depots[d] || depots[d]->V < 2)  // Search for a free depot
                            break;
                    assert(d < Salesmen);
                    assert(depots[d]);
                    (currentPetal->depTail = depots[d])->V++;
                }
                // Attach depot at the end of the current petal
                Link(currentPetal->tail, currentPetal->depTail);
                currentPetal->tail = currentPetal->depTail;
                assert(currentPetal->tail->DepotId);

                if (currentPetal->tail->V == 1) {  // Current chain ends with depot, but no other chain requires it
                    // Find chain with a not costrained end
                    for (int c = 0; c < Salesmen; ++c) {
                        Petal *p = &petals[c];
                        if (!p->taken) {
                            if (p->depHead && !p->depTail) {  // reverse petal
                                reversePetal(p);
                                assert(!p->depHead);
                                assert(p->depTail);
                                assert(!p->head->Pred);
                                assert(!p->tail->Suc);
                            }
                            if (!p->depHead) {  // then just Link
                                (p->depHead = currentPetal->tail)->V++;
                                Link(currentPetal->tail, p->head);
                                p->taken = 1;
                                currentPetal = p;
                                ++addedPetals;
                                break;
                            }
                        }
                    }
                } else if (currentPetal->tail->V == 2) {  // Current chain ends with depots and another chain that needs it exists.
                    for (int c = 0; c < Salesmen; ++c) {
                        Petal *p = &petals[c];
                        if (!p->taken) {
                            if (p->depTail == currentPetal->tail) {  // reverse petal
                                reversePetal(p);
                                assert(!p->head->Pred);
                                assert(!p->tail->Suc);
                            }
                            if (p->depHead == currentPetal->tail) {  // link
                                Link(currentPetal->tail, p->head);
                                p->taken = 1;
                                currentPetal = p;
                                ++addedPetals;
                                break;
                            }
                        }
                    }
                }
            } while (addedPetals < Salesmen);

            assert(!((currentPetal->depTail && initialPetal->depHead) && (currentPetal->depTail != initialPetal->depHead)));

            if (currentPetal->depTail) {
                Link(currentPetal->tail, currentPetal->depTail);
                Link(currentPetal->depTail, initialPetal->head);
            } else {  // both NULL
                // search for the last FREEN0 depot
                for (int d = Salesmen - 1; d >= 0; --d) {
                    if (depots[d] && !depots[d]->V) {  // V must be 0
                        Link(currentPetal->tail, depots[d]);
                        Link(depots[d], initialPetal->head);
                        break;
                    }
                }
            }

            FREEN0(depots);
            FREEN0(petals);
        } else {
            int depIndex = Dim + 1;
            Node *Pred = NodeSet + tour[0];
            Node *N;
            for (int i = 1; i <= DimensionSaved; ++i) {
                if ((tour[i] == MTSPDepot) && (depIndex <= DimensionSaved))  // the last depot remains = MTSPDepot
                    tour[i] = depIndex++;

                N = NodeSet + tour[i];
                Link(Pred, N);
                Pred = N;
            }
        }

        // Check tour
        Node *N = Depot;
        memset(tour, 0, sizeof(int) * (DimensionSaved + 1));
        do {
            assert(N->Id <= DimensionSaved && N->Id > 0);
            tour[N->Id]++;
            assert(N == N->Suc->Pred);
            assert(N == N->Pred->Suc);
            assert(!DepotCheckForbidden(N, N->Suc));
        } while ((N = N->Suc) != Depot);

        int index = 1;
        do {
            assert(tour[index] == 1);  // check tour
            tour[index++] = N->Id;
            N->V = 0;
            N->LastV = 0;
        } while ((N = N->Suc) != Depot);
        tour[0]=tour[DimensionSaved];

        assert(index == DimensionSaved + 1);
    }
}

void reversePetal(Petal *p) {
    Node *N = p->head, *Suc;

    while (N != p->tail) {  // first reverse chain
        Suc = N->Suc;
        assert(Suc && N);
        N->Suc = N->Pred;
        N->Pred = Suc;
        N = Suc;
    }

    // Swap also the last node
    Suc = N->Suc;
    N->Suc = N->Pred;
    N->Pred = Suc;

    // Swap depots
    Suc = p->depHead;
    p->depHead = p->depTail;
    p->depTail = Suc;

    // Swap head with tail
    Suc = p->head;
    p->head = p->tail;
    p->tail = Suc;
}

int DepotCheckForbidden(Node *Na, Node *Nb) {
    if (Salesmen > 1 && Dimension == DimensionSaved) {
        if (Na->DepotId) {
            if ((Nb->DepotId && MTSPMinSize >= 1) ||
                (Nb->Special && Nb->Special != Na->DepotId && Nb->Special != Na->DepotId % Salesmen + 1))
                return 1;
        }
        if (Nb->DepotId) {
            if ((Na->DepotId && MTSPMinSize >= 1) ||
                (Na->Special && Na->Special != Nb->DepotId && Na->Special != Nb->DepotId % Salesmen + 1))
                return 1;
        }
    }
    return 0;
}