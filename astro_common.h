//
// Created by 李林峰 on 2019-06-25.
//

#ifndef ASTROWEATHER_ASTRO_COMMON_H
#define ASTROWEATHER_ASTRO_COMMON_H

#include "astro.h"

double EpochToEphemTime(double seconds_since_epoch);
double EphemToEpochTime(double ephem);
int GetModifiedRiset(Now *now, int index, RiseSet *riset);
const char *GetStarName(int index);

#endif //ASTROWEATHER_ASTRO_COMMON_H
