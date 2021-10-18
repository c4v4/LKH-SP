#include "LKH.h"

/*
 * The Forbidden function is used to test if an edge, (Na,Nb),
 * is not allowed to belong to a tour.
 * If the edge is forbidden, the function returns 1; otherwise 0.
 */
int Forbidden(Node *Na, Node *Nb) {
    if (Na == Nb) return 1;
    if (/* InInitialTour(Na, Nb) ||  */ Na->Id == 0 || Nb->Id == 0) return 0; /*J&V MTSP->TSP transform applied, no need to check InitialTour*/
    if (Asymmetric && (Na->Id <= DimensionSaved) == (Nb->Id <= DimensionSaved)) return 1;
    if (Na->Head && !FixedOrCommon(Na, Nb) && (Na->Head == Nb->Head || (Na->Head != Na && Na->Tail != Na) || (Nb->Head != Nb && Nb->Tail != Nb))) return 1;
    return 0;
}