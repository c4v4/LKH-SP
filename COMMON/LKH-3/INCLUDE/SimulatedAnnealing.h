#ifndef _SIMULATEDANNEALING_H
#define _SIMULATEDANNEALING_H

#include "LKH.h"

void SA_setup(double EntryTime, double TimeLimit);
void SA_start(void);
int SA_test(GainType Pnlt, GainType Cost);

#endif