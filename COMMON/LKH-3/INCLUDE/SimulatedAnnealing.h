#ifndef _SIMULATEDANNEALING_H
#define _SIMULATEDANNEALING_H

#include "LKH.h"

void SA_start(double TimeLeft);
int SA_test(GainType Pnlt, GainType Cost);
void SA_update(void);


#endif