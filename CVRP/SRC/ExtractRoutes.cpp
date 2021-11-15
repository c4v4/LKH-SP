#include "extract_routes.hpp"

extern "C" {
#include "LKH.h"
#include "Segment.h"
};

class CVRPChecker : public ConstraintChecker {
public:
    void add_node(Node *N) {
        current_demand += N->Demand;
        ConstraintChecker::add_node(N);
    };

    void clear_route(Node *Prev) {
        current_demand = 0;
        ConstraintChecker::clear_route(Prev);
    };

    int is_feasible() const { return current_demand <= Capacity && ConstraintChecker::is_feasible(); }
    inline GainType get_demand() const { return current_demand; };

private:
    GainType current_demand = 0;
};

extern "C" int ExtractRoutes(GainType Cost) { return extract_routes_tmlp<CVRPChecker>(Cost); }