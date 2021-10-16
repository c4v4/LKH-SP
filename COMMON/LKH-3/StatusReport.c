#include "LKH.h"

void StatusReport(GainType Cost, double EntryTime, const char * Suffix)
{
    printff("Cost = " GainFormat "_" GainFormat, CurrentPenalty, Cost);
    if (Optimum != MINUS_INFINITY && Optimum != 0) {
        if (ProblemType != CCVRP && ProblemType != TRP &&
            ProblemType != MLP &&
            MTSPObjective != MINMAX && MTSPObjective != MINMAX_SIZE)
            printff(", Gap = %0.4f%%",
                    100.0 * (Cost - Optimum) / Optimum);
        else
            printff(", Gap = %0.4f%%",
                    100.0 * (CurrentPenalty - Optimum) / Optimum);
    }
    printff(", Time = %0.2f sec. %s",
            fabs(GetTime() - EntryTime), Suffix);
    
    printff("\n");
}
