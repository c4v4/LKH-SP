#include "LKH.h"

void SetInitialTour(int *warmstart) {

    if (warmstart) {
        ValidateTour(warmstart);
        Node *N = Depot, *NextN;
        do {
            NextN = N->Suc;
            assert(N->Id <= DimensionSaved);
            assert(NextN->Id <= DimensionSaved);
            if (Asymmetric) {
                Node *TwinN = N + DimensionSaved;
                TwinN->InitialSuc = N;
                Link(TwinN, N);
                N->InitialSuc = NextN + DimensionSaved;
                Link(N, NextN + DimensionSaved);
                if (Forbidden(TwinN, N))
                    eprintf("%d, %d\n", TwinN->Id, N->Id);
                if (Forbidden(N, N->InitialSuc))
                    eprintf("%d, %d\n", N->Id, N->InitialSuc->Id);
            } else
                N->InitialSuc = NextN;
        } while ((N = NextN) != Depot);
        assert(Depot->InitialSuc);
        AddTourCandidates();
    }
}