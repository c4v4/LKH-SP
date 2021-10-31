#include <cassert>
#include <vector>

#include "SPH.hpp"

extern "C" {
#ifdef NDEBUG
#define OLD_NDEBUG
#endif

#include "LKH.h"

#ifdef OLD_NDEBUG
#define NDEBUG
#endif
}


extern void *sph_ptr;
extern std::vector<sph::idx_t> BestRoutes; /* Vector containing routes indexes of BestTour */


class ConstraintChecker {
public:
    void add_node(Node *N) {
        // dist = OldDistance(prev, N + (DimensionSaved * Asymmetric));
        Node *realN = N + (DimensionSaved * Asymmetric);
        dist = (C(prev, realN) - prev->Pi - realN->Pi) / Precision;
        current_length += dist;
        prev = N;
        ++size;
    };

    void clear_route(Node *Prev) {
        current_length = 0;
        size = 0;
        prev = Prev;
    };

    int is_feasible() const { return size >= MTSPMinSize + 1; }
    inline GainType get_length() const { return current_length; };
    inline size_t get_size() const { return size; };
    inline int get_dist() const { return dist; };

protected:
    int dist = 0;
    Node *prev = Depot;
    GainType current_length = 0;
    size_t size = 0;
};

/**
 * @brief Template function the extracts feasible routes from a solution.
 *
 * @tparam ConstrChecker: Class that checks route feasibility
 */
template <typename ConstrChecker>
void extract_routes_tmlp(GainType Cost) {
    static GainType best_cost = Cost + 1;

    bool store_best = Cost < best_cost && CurrentPenalty == 0;
    if (store_best) {
        best_cost = Cost;
        BestRoutes.clear();
    }

    static std::vector<sph::idx_t> current_route;

    sph::SPHeuristic &sph = *((sph::SPHeuristic *)sph_ptr);
    ConstrChecker check;

    if (CurrentPenalty && ProblemType != CCVRP)
        Cost = INT_MAX;


    int count_infeas = 0;
    int count_empty = 0;
    GainType CostCheck = 0;
    Node *N = Depot;
    check.clear_route(Depot);

    auto next_N = (!Asymmetric                    ? [](Node *N) { return N->Suc; }
                   : N->Suc != N + DimensionSaved ? [](Node *N) { return N->Suc->Suc; }
                                                  : [](Node *N) { return N->Pred->Pred; });
    do {
        N = next_N(N);
        assert(N->Id <= DimensionSaved);
        check.add_node(N);
        if (N->DepotId) {
            if (check.is_feasible()) {
                sph::idx_t col_idx = sph.add_column(current_route.begin(), current_route.end(), check.get_length(), Cost);
                if (store_best)
                    BestRoutes.push_back(col_idx);
            } else {
                ++count_infeas;
                assert(CurrentPenalty > 0);
            }
            count_empty += check.get_size() == 1;
            current_route.clear();
            CostCheck += check.get_length();
            check.clear_route(N);
        } else
            current_route.push_back(N->Id - 2);
    } while (N != Depot);

    if (store_best && TraceLevel >= 0) {
        printff("** ");
        if (MTSPMinSize == 0)
            printff("Vehicles = %d, ", Salesmen - count_empty);
        StatusReport(Cost, StartTime, "");
    }

    assert(CurrentPenalty > 0 || CostCheck == Cost);
    assert((count_infeas > 0) == (CurrentPenalty > 0) || ProblemType == CCVRP);
}