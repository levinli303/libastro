//
// Created by Levin Li on 2019-06-25.
//

#pragma once

#include <string>
#include <vector>

extern "C" {
#include "astro.h"
}

#define STATUS_NIGHT                  1
#define STATUS_ASTRONOMICAL_TWILIGHT  2
#define STATUS_NAUTICAL_TWILIGHT      3
#define STATUS_BLUE_HOUR_TWILIGHT     4
#define STATUS_CIVIL_TWILIGHT         5
#define STATUS_GOLDEN_HOUR_TWILIGHT   6
#define STATUS_DAY                    7
#define STATUS_GOLDEN_HOUR_DUSK       8
#define STATUS_CIVIL_DUSK             9
#define STATUS_BLUE_HOUR_DUSK         10
#define STATUS_NAUTICAL_DUSK          11
#define STATUS_ASTRONOMICAL_DUSK      12
#define STATUS_GOLDEN_HOUR_UNKNOWN    13
#define STATUS_CIVIL_UNKNOWN          14
#define STATUS_BLUE_HOUR_UNKNOWN      15
#define STATUS_NAUTICAL_UNKNOWN       16
#define STATUS_ASTRONOMICAL_UNKNOWN   17

struct TimePeriod {
    double start;
    double end;
    int status;
};

double radian(const double degree);
double EpochToEphemTime(double seconds_since_epoch);
double EphemToEpochTime(double ephem);
int GetModifiedRisetS(Now *now, Obj *obj, double step, double limit, RiseSet *riset, double *el, double *az, bool up);
int GetModifiedRiset(Now *now, int index, RiseSet *riset, double *el, double *az, bool up);
const char *GetStarName(int index);
void ConfigureObserver(double longitude, double latitude, double altitude, double seconds_since_epoch, Now *obj);
double FindMoonPhase(double seconds_since_epoch, double motion, double target);
double CurrentMoonPhase(double seconds_since_epoch);
void GetRADECRiset(double ra, double dec, double longitude, double latitude, double now, double *riseTime, double *setTime, double *transitTime, int *status, double *az_r, double *az_s, double *az_c, double *az_t, double *el_c, double *el_t);
double GetLST(double now, double longitude);
int FindAltXSun(Now *now, double step, double limit, int forward, int go_down, double *jd, double x);
int GetSatellitePosition(const char* line0, const char* line1, const char* line2, double longitude, double latitude, double altitude, double seconds_since_epoch, double* el, double* az);
int GetSatelliteStatus(const char* line0, const char* line1, const char* line2, double seconds_since_epoch, double* sublng, double* sublat, double* elevation);
int GetNextSatellitePass(const char* line0, const char* line1, const char* line2, double seconds_since_epoch, double longitude, double latitude, double altitude, RiseSet* riset, RiseSet* visibleRiset, double* visibleRiseAlt, double* visibleSetAlt);
std::vector<TimePeriod> GetSunDetails(double longitude, double latitude, double altitude, double startTime, double endTime);

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
