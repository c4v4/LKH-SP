#include <cassert>
#include <vector>

#include "SPH.hpp"

extern "C" {
#include "LKH.h"
}


extern void *sph_ptr;

/**
 * @brief Template function the extracts feasible routes from a solution.
 *
 * @tparam ConstraintChecker: Class that checks route feasibility
 */
template <typename ConstraintChecker>
void extract_routes_tmlp(GainType Cost) {
    static thread_local std::vector<sph::idx_t> current_route;

    sph::SPHeuristic &sph = *((sph::SPHeuristic *)sph_ptr);
    ConstraintChecker check;

    if (CurrentPenalty && ProblemType != CCVRP) Cost = INT_MAX;

    int count_infeas = 0;
    check.clear_route(Depot);

    Node *N = Depot;
    while ((N = N->Suc) != Depot) {
        if (N->Id > DimensionSaved) continue;

        if (N->DepotId) {
            if (check.is_feasible()) {
                sph.add_column(current_route.begin(), current_route.end(), check.get_length(), Cost);
            } else {
                ++count_infeas;
                assert(CurrentPenalty > 0);
            }
            current_route.clear();
            check.clear_route(N);
        } else
            current_route.push_back(N->Id - 2);  // ########## HERE, if something is wrong check this first!!

        check.add_node(N);
    }

    assert((count_infeas > 0) == (CurrentPenalty > 0) || ProblemType == CCVRP);
}