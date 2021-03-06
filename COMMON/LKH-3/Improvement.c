#include "LKH.h"
#include "Segment.h"

/* The Improvement function is used to check whether a done move
 * has improved the current best tour.
 * If the tour has been improved, the function computes the penalty gain
 * and returns 1. Otherwise, the move is undone, and the function returns 0.
 */

int Improvement(GainType* Gain, Node* t1, Node* SUCt1) {
    GainType NewPenalty;

    CurrentGain = *Gain;
    NewPenalty = Penalty();
    if (NewPenalty <= CurrentPenalty) {
        if (TSPTW_Makespan)
            *Gain = (TSPTW_CurrentMakespanCost - TSPTW_MakespanCost()) * Precision;
        if (NewPenalty < CurrentPenalty || *Gain > 0) {
            PenaltyGain = CurrentPenalty - NewPenalty;

            /* GainType storedP = 0;
            for (int i = 0; i <= Salesmen; ++i) {
                storedP += cava_PetalsData[i].OldPenalty;
            }
            assert(storedP == NewPenalty); */
#ifdef CAVA_FLIP
            cava_FlipAsym_Update();
#endif

            return 1;
        }
    }
    RestoreTour();
    if (SUC(t1) != SUCt1)
        Reversed ^= 1;
    *Gain = PenaltyGain = 0;
    return 0;
}
