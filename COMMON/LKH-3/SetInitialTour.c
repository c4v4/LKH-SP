#include "LKH.h"

void SetInitialTour(int *warmstart) {

    if (warmstart) {
        ValidateTour(warmstart);
        Node *N = Depot;
        do {
            if (Asymmetric) {
                Node *Na = N + DimensionSaved;
                Na->InitialSuc = N;
                N->InitialSuc = N->Suc + DimensionSaved;

                if (Forbidden(Na, N)) eprintf("%d, %d\n", Na->Id, N->Id);

                if (Forbidden(N, N->InitialSuc)) eprintf("%d, %d\n", N->Id, N->InitialSuc->Id);
            } else
                N->InitialSuc = N->Suc;
        } while ((N = N->Suc) != Depot);

        AddTourCandidates();
    }
}