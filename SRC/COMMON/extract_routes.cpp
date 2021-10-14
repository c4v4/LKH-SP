#include <vector>
#include <cassert>

#include "SPH.hpp"

extern "C" {
#include "LKH.h"
}

static std::vector<int> current_route;

/**
 * @brief Template function the extracts feasible routes from a solution.
 *
 * @tparam ConstraintChecker: Class that checks route feasibility
 */
template <typename ConstraintChecker>
void extract_routes_tmlp() {
    SPHeuristic &sph = *((SPHeuristic *)sph_ptr);
    ConstraintChecker check;

    if (CurrentPenalty && ProblemType != CCVRP) Cost = INT_MAX;

    int count_infeas = 0;
    check.clear_route(Depot);

    Node *N = Depot;
    while ((N = N->Suc) != Depot) {
        if (N->Id > DimensionSaved) continue;

        if (N->DepotId) {
            if (check.is_feasible()) {
                auto dem = check.get_demand();
                auto len = check.get_length();
                sph.add_column(current_route.begin(), current_route.end(), len, Cost);
            } else {
                ++count_infeas;
                assert(CurrentPenalty > 0);
            }
            current_route.clear();
            check.clear_route(client);
        } else
            current_route.push_back(ID(client));

        check.add_node(client);
    }

    assert((count_infeas > 0) == (CurrentPenalty > 0) || ProblemType == CCVRP);
}