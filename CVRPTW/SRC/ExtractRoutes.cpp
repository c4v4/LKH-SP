#include "extract_routes.hpp"

extern "C" {
#include "LKH.h"
#include "Segment.h"
};

class CvrptwChecker : public ConstraintChecker {
public:
    inline void add_node(Node *N) {
        ConstraintChecker::add_node(N);
        current_demand += N->Demand;
        auto real_cost = current_cost + get_length();
        if (real_cost < N->Earliest)
            current_cost = static_cast<GainType>(N->Earliest) - get_length();
        if (real_cost > N->Latest)
            current_cost = INT_MAX;
        current_cost += N->ServiceTime;
    }

    inline void clear_route(Node *Prev) {
        ConstraintChecker::clear_route(Prev);
        current_cost = current_demand = 0;
    }

    inline int is_feasible() const {
        return ConstraintChecker::is_feasible() && current_demand <= Capacity && current_cost + get_length() <= Depot->Latest &&
               get_size() >= static_cast<size_t>(MTSPMinSize);
    }

    inline GainType get_cost() const { return current_cost + get_length(); };

    inline GainType get_demand() const { return current_demand; };

private:
    GainType current_demand = 0;
    GainType current_cost = 0;
};


extern "C" void ExtractRoutes(GainType Cost) { extract_routes_tmlp<CvrptwChecker>(Cost); }