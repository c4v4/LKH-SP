#include "LKH.h"

/*
 * The Forbidden function is used to test if an edge, (Na,Nb),
 * is not allowed to belong to a tour.
 * If the edge is forbidden, the function returns 1; otherwise 0.
 */
int Forbidden(Node *Na, Node *Nb) {
    if (Na == Nb)
        return 1;
    if (/* InInitialTour(Na, Nb) ||  */ Na->Id == 0 || Nb->Id == 0)
        return 0; /*J&V MTSP->TSP transform applied, no need to check InitialTour*/
    if (Na->Head && !FixedOrCommon(Na, Nb) &&
        (Na->Head == Nb->Head || (Na->Head != Na && Na->Tail != Na) || (Nb->Head != Nb && Nb->Tail != Nb)))
        return 1;
    if (Na->DepotId && Nb->DepotId && MTSPMinSize > 0) {
        return 1;
    }
    if (Salesmen > 1) {
        if (Na->DepotId) {
            if (Nb->Special && Nb->Special != Na->DepotId && Nb->Special != Na->DepotId % Salesmen + 1)
                return 1;
        }
        if (Nb->DepotId) {
            if (Na->Special && Na->Special != Nb->DepotId && Na->Special != Nb->DepotId % Salesmen + 1)
                return 1;
        }
    }
    return 0;
}