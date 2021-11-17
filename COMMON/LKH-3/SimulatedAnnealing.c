#include "LKH.h"

#define SIMULATED_ANNEALING
#ifdef SIMULATED_ANNEALING
#define SA_ZERO_FACTOR 250 /* Temperature multiplicative factor when no initial solution is available */
#define SA_WARM_FACTOR 50  /* Temperature multiplicative factor when a initial solution is available */
#else
#define SA_ZERO_FACTOR 0
#define SA_WARM_FACTOR 0
#endif

#define SA_SCALING_FACTOR 1E-5
#define T_0_cost (BetterCost * SA_SCALING_FACTOR)
#define T_0_pnlt (BetterPenalty * SA_SCALING_FACTOR)
#define lnU_01() (log((double)rand() / (double)RAND_MAX))

#if (SA_ZERO_FACTOR > 0) || (SA_WARM_FACTOR > 0)

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

    if (Dim <= 10000) {
        if (Run <= SphPeriod && !InitialTourFileName)
            T = T_trials = T_time = SA_ZERO_FACTOR * SA_SCALING_FACTOR /** (1 + (Run - 1) % SphPeriod)*/;
        else
            T = T_trials = T_time = SA_WARM_FACTOR * SA_SCALING_FACTOR /** (1 + (Run - 1) % SphPeriod)*/;
    }
    cost_delta = T * BetterCost;
    pnlt_delta = T * BetterPenalty;
    if (TraceLevel >= 1)
        printff(" Initial Temperature T0_cost = %.2f, T0_pnlt = %.2f\n", cost_delta, pnlt_delta);
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