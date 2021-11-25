#include "LKH.h"

#define SIMULATED_ANNEALING
#ifdef SIMULATED_ANNEALING
#define SA_ZERO_SCALE 100 /* Temperature multiplicative factor when no initial solution is available */
#define SA_WARM_SCALE 10  /* Temperature multiplicative factor when an initial solution is available */
#else
#define SA_ZERO_SCALE 0
#define SA_WARM_SCALE 0
#endif

#define SA_SCALE 1E-5
#define lnU_01() (log((double)rand() / (double)RAND_MAX))

#if (SA_ZERO_SCALE > 0) || (SA_WARM_SCALE > 0)

static double prev_time;
static double T;
static double T_trials;
static double T_time;
static double cost_delta;
static double pnlt_delta;
static double trials_step;
static double timelimit;

void SA_setup(double EntryTime, double TimeLimit) {
    prev_time = GetTime();
    T = T_trials = T_time = cost_delta = pnlt_delta = 0.0;
    trials_step = pow(0.01, 1.0 / (double)MaxTrials);
    timelimit = TimeLimit - EntryTime;
}

void SA_start() {

    if (Dim <= 2000) {
        T = (Run <= SphPeriod && !InitialTourFileName) ? SA_ZERO_SCALE : SA_WARM_SCALE;
        T *= SA_SCALE * SAFactor * (1 + (Run - 1) % SphPeriod);
        T_trials = T_time = T;
    }
    cost_delta = T * BetterCost;
    pnlt_delta = T * BetterPenalty;
    if (TraceLevel >= 1)
        printff(" SA factor = %.1e, T_cost = [%.2f, %.2f], T_pnlt = [%.2f, %.2f]\n", T, cost_delta, cost_delta / 100.0, pnlt_delta,
                pnlt_delta / 100.0);
}

void SA_update() {

    /* Iteration based SA */
    T_trials *= trials_step;

    /* Time based SA */
    double now = GetTime();
    double lasted_time = now - prev_time;
    T_time *= pow(0.01, lasted_time / timelimit);

    /* Keep the lowest of the two*/
    T = T_trials < T_time ? T_trials : T_time;

    cost_delta = T * BetterCost;
    pnlt_delta = T * BetterPenalty;
    prev_time = now;
}

int SA_test(GainType Pnlt, GainType Cost) {
    if (T > 0) {
        SA_update();
        GainType temp_cost = BetterCost - cost_delta * lnU_01();
        GainType temp_pnlt = BetterPenalty - pnlt_delta * lnU_01();
        return Pnlt < BetterPenalty || (Pnlt <= temp_pnlt && Cost < temp_cost && Cost != BetterCost);
    }
    return Pnlt < BetterPenalty || (Pnlt == BetterPenalty && Cost < BetterCost);
}
#else

void SA_setup(double EntryTime, double TimeLimit) { }
void SA_start() { }
int SA_test(GainType Pnlt, GainType Cost) { return Pnlt < BetterPenalty || (Pnlt == BetterPenalty && Cost < BetterCost); }

#endif
