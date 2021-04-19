//
// Created by 李林峰 on 2019-06-25.
//

#ifndef ASTROWEATHER_ASTRO_COMMON_H
#define ASTROWEATHER_ASTRO_COMMON_H

#include <string>

extern "C" {
#include "astro.h"
}

double radian(const double degree);
double EpochToEphemTime(double seconds_since_epoch);
double EphemToEpochTime(double ephem);
int GetModifiedRiset(Now *now, int index, RiseSet *riset, double *el, double *az);
const char *GetStarName(int index);
void ConfigureObserver(double longitude, double latitude, double altitude, double seconds_since_epoch, Now *obj);
double FindMoonPhase(double seconds_since_epoch, double motion, double target);
void GetRADECRiset(double ra, double dec, double longitude, double latitude, double now, double *riseTime, double *setTime, double *transitTime, int *status, double *az_r, double *az_s, double *az_c, double *az_t, double *el_c, double *el_t);
double GetLST(double now, double longitude);

namespace astro
{
    class Date
    {
    public:
        Date();
        Date(int Y, int M, int D);
        Date(double);

        enum Format
        {
            Locale          = 0,
            TZName          = 1,
            UTCOffset       = 2,
            ISO8601         = 3,
        };

        const char* toCStr(Format format = Locale) const;

        operator double() const;

        static Date systemDate();


    public:
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int wday;           // week day, 0 Sunday to 6 Saturday
        int utc_offset;     // offset from utc in seconds
        std::string tzname; // timezone name
        double seconds;
    };
}

#endif //ASTROWEATHER_ASTRO_COMMON_H
