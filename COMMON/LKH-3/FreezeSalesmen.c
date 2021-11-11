#include "LKH.h"


void FreezeSalesmen() {
    if (MTSPMinSize > 0)
        return;

    Node *N, *NextN;
    for (int i = 0; i < DimensionSaved; ++i) {
        N = NodeSet + BetterTour[i];
        NextN = NodeSet + BetterTour[i + 1] + !!Asymmetric * DimensionSaved;
        if (N->DepotId && NextN->DepotId) {
            assert(N->DepotId != NextN->DepotId);
            assert(N->FixedTo2 == NULL || N->FixedTo2 == NextN);
            assert(NextN->FixedTo2 == NULL || NextN->FixedTo2 == N);
            (N->FixedTo2 = NextN)->FixedTo2 = N;
        }
    }
    MTSPMinSize = 1;
}

void UnfreezeSalesmen() {
    if (MTSPMinSize == 0)
        return;

    for (int i = Dim + 1; i <= DimensionSaved; ++i) {
        assert(NodeSet[i].FixedTo2 == NULL || (NodeSet[i].FixedTo2->DepotId && NodeSet[i].DepotId != NodeSet[i].FixedTo2->DepotId));
        NodeSet[i].FixedTo2 = NULL;
    }
    if (Asymmetric)
        for (int i = DimensionSaved + Dim + 1; i <= Dimension; ++i) {
            assert(NodeSet[i].FixedTo2 == NULL || (NodeSet[i].FixedTo2->DepotId && NodeSet[i].DepotId != NodeSet[i].FixedTo2->DepotId));
            NodeSet[i].FixedTo2 = NULL;
        }
    MTSPMinSize = 0;
}