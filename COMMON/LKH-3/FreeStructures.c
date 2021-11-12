#include "Genetic.h"
#include "LKH.h"
#include "Sequence.h"

/*
 * The FreeStructures function frees all allocated structures.
 */

#define Free(s)  \
    {            \
        free(s); \
        s = 0;   \
    }

void FreeStructures() {
    FreeCandidateSets();
    FreeSegments();
    if (NodeSet) {
        int i;
        for (i = 1; i <= Dimension; i++) {
            Node *N = &NodeSet[i];
            Free(N->MergeSuc);
            N->C = 0;
        }
        Free(NodeSet);
    }
    Free(CostMatrix);
    Free(BestTour);
    Free(BetterTour);
    Free(SwapStack);
    Free(HTable);
    Free(Rand);
    Free(CacheSig);
    Free(CacheVal);
    Free(Name);
    Free(Type);
    Free(EdgeWeightType);
    Free(EdgeWeightFormat);
    Free(EdgeDataFormat);
    Free(NodeCoordType);
    Free(DisplayDataType);
    Free(Heap);
    Free(t);
    Free(T);
    Free(tSaved);
    Free(p);
    Free(q);
    Free(incl);
    Free(cycle);
    Free(G);
    FreePopulation();

#ifdef CAVA_PENALTY
    Free(cava_PetalsData);
    Free(cava_NodeCache);
#endif
}

/*
   The FreeSegments function frees the segments.
 */

void FreeSegments() {
    if (FirstSegment) {
        Segment *S = FirstSegment, *SPrev;
        do {
            SPrev = S->Pred;
            Free(S);
        } while ((S = SPrev) != FirstSegment);
        FirstSegment = 0;
    }
    if (FirstSSegment) {
        SSegment *SS = FirstSSegment, *SSPrev;
        do {
            SSPrev = SS->Pred;
            Free(SS);
        } while ((SS = SSPrev) != FirstSSegment);
        FirstSSegment = 0;
    }
}

/*
 * The FreeCandidateSets function frees the candidate sets.
 */

void FreeCandidateSets() {
    Node *N = FirstNode;
    if (!N)
        return;
    for (int i = 1; i <= Dimension; i++) {
        Free(NodeSet[i].CandidateSet);
        Free(NodeSet[i].BackboneCandidateSet);
    }
}
