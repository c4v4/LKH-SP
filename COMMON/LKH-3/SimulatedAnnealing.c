#include "LKH.h"

//#define SIMULATED_ANNEALING
#ifdef SIMULATED_ANNEALING
#define SA_ZERO_FACTOR 250 /* Temperature multiplicative factor when no initial solution is available */
#define SA_WARM_FACTOR 50  /* Temperature multiplicative factor when a initial solution is available */
#else
#define SA_ZERO_FACTOR 0
#define SA_WARM_FACTOR 0
#endif

#define SA_SCALING_FACTOR 0.00001
#define T_0_cost (BetterCost * SA_SCALING_FACTOR)
#define T_0_pnlt (BetterPenalty * SA_SCALING_FACTOR)
#define lnU_01() (log((double)rand() / (double)RAND_MAX))
#define START_FROM 0

#if (SA_ZERO_FACTOR > 0) || (SA_WARM_FACTOR > 0)

static double prev_time;
static double T;
static double T_trials;
static double T_time;
static double cost_delta;
static double pnlt_delta;
static double trials_step;
static double timelimit;

void SA_start(double TimeLeft) {
    if (Run < START_FROM)
        return;

    T = T_trials = T_time = cost_delta = pnlt_delta = 0.0;
    trials_step = pow(0.01, 1.0 / (double)MaxTrials);
    timelimit = TimeLeft;
    prev_time = GetTime();

    if (Dim <= 2000) {
        if (FirstNode->InitialSuc) {
            T = T_trials = T_time = SA_WARM_FACTOR * SA_SCALING_FACTOR;
        } else {
            T = T_trials = T_time = SA_ZERO_FACTOR * SA_SCALING_FACTOR;
        }
    }
    cost_delta = T * BetterCost;
    pnlt_delta = T * BetterPenalty;
    if (TraceLevel >= 1)
        printff(" Initial Temperature T0_cost = %.2f, T0_pnlt = %.2f\n", cost_delta, pnlt_delta);
}

void SA_update() {
    if (Run < START_FROM)
        return;

    double lasted_time = GetTime() - prev_time;
    prev_time += lasted_time;

    T_time *= pow(0.01, lasted_time / timelimit); /* Time based SA */
    T_trials *= trials_step;                      /* Iteration based SA */
    T = T_trials < T_time ? T_trials : T_time;    /* Keep the lowest of the two*/
    cost_delta = T * BetterCost;
    pnlt_delta = T * BetterPenalty * 30;
}

int SA_test(GainType Pnlt, GainType Cost) {
    SA_update();
    GainType temp_cost = BetterCost;
    GainType temp_pnlt = BetterPenalty;
    if (Run >= START_FROM) {
        temp_cost -= cost_delta * lnU_01();
        temp_pnlt -= pnlt_delta * lnU_01();
    }
    return CurrentPenalty < BetterPenalty || (CurrentPenalty <= temp_pnlt && Cost < temp_cost && Cost != BetterCost);
}
#else

void SA_start(double TimeLeft) { }
void SA_update() { }
int SA_test(GainType Pnlt, GainType Cost) {
    return CurrentPenalty < BetterPenalty || (CurrentPenalty == BetterPenalty && Cost < BetterCost);
}

#endif