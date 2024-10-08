//
// Created by Levin Li on 2019-06-25.
//

#include "astro_common.h"
#include <cstring>
#include <cmath>
#include <ctime>
#include <iostream>
#include <utility>

#define EPHEM_SECONDS_DIFFERENCE   2209032000

#define SECONDS_PER_DAY 86400.0
#define MINUTES_PER_DAY 1440.0
#define HOURS_PER_DAY   24.0

using namespace std;

static const char *planetNames[] = {
        "Mercury",
        "Venus",
        "Mars",
        "Jupiter",
        "Saturn",
        "Uranus",
        "Neptune",
        "Pluto",
        "Sun",
        "Moon",
};

double radian(double degree)
{
    return degree / 180.0 * M_PI;
}

const char *GetStarName(int index)
{
    return planetNames[index];
}

double EpochToEphemTime(double seconds_since_epoch)
{
    return (seconds_since_epoch + EPHEM_SECONDS_DIFFERENCE) / 86400;
}

double EphemToEpochTime(double ephem)
{
    return ephem * 86400 - EPHEM_SECONDS_DIFFERENCE;
}

int FindAltX(Now *now, Obj *obj, double step, double limit, int forward, int go_down, double *az, double *jd, double *transit_az, double *transit_al, double *transit_tm, double x)
{
    double orig = now->n_mjd;
    double current = orig;

    Obj backup;
    memcpy(&backup, obj, sizeof(Obj));

    obj_cir(now, &backup);

    double prev_az = backup.pl.co_az;
    double prev_alt = backup.pl.co_alt;

    if (prev_alt > *transit_al)
    {
        *transit_al = prev_alt;
        *transit_az = prev_az;
        *transit_tm = orig;
    }

    double prev_time = current;

    memcpy(&backup, obj, sizeof(Obj));

    go_down = forward ? go_down : !go_down;

    for (;;)
    {
        now->n_mjd = current;
        obj_cir(now, &backup);

        double curr_alt = backup.pl.co_alt;
        double curr_az = backup.pl.co_az;

        if (curr_alt > *transit_al)
        {
            *transit_al = curr_alt;
            *transit_az = curr_az;
            *transit_tm = current;
        }

        memcpy(&backup, obj, sizeof(Obj));

        if (go_down)
        {
            if (prev_alt >= x && curr_alt <= x)
            {
                *az = (prev_az + curr_az) / 2;
                *jd = (prev_time + current) / 2;
                now->n_mjd = orig;
                return 0;
            }
        }
        else
        {
            if (prev_alt <= x && curr_alt >= x)
            {
                *az = (prev_az + curr_az) / 2;
                *jd = (prev_time + current) / 2;
                now->n_mjd = orig;
                return 0;
            }
        }

        prev_alt = curr_alt;
        prev_az = curr_az;
        prev_time = current;

        if (forward)
        {
            if (current > orig + limit)
            {
                break;
            }
        }
        else
        {
            if (current < orig - limit)
            {
                break;
            }
        }
        forward ? current += step : current -= step;
    }
    now->n_mjd = orig;
    return 1;
}

int FindAlt0(Now *now, Obj *obj, double step, double limit, int forward, int go_down, double *az, double *jd, double *transit_az, double *transit_al, double *transit_tm)
{
    return FindAltX(now, obj, step, limit, forward, go_down, az, jd, transit_az, transit_al, transit_tm, 0);
}

int FindAltXSun(Now *now, double step, double limit, int forward, int go_down, double *jd, double x)
{
    double az, transit_az, transit_al, transit_tm;
    Obj *objs;
    getBuiltInObjs(&objs);

    Obj origObj = objs[SUN];
    return FindAltX(now, &origObj, step, limit, forward, go_down, &az, jd, &transit_az, &transit_al, &transit_tm, x);
}

int GetModifiedRiset(Now *now, int index, RiseSet *riset, double *el, double *az, bool up)
{
    Obj *objs;
    getBuiltInObjs(&objs);

    Obj origObj = objs[index];
    return GetModifiedRisetS(now, &origObj, 1.0 / 1440, 1.0, riset, el, az, up);
}

int GetModifiedRisetS(Now *now, Obj *obj, double step, double limit, RiseSet *riset, double *el, double *az, bool up)
{
    Now backup;
    memcpy(&backup, now, sizeof(Now));

    Obj newObj;
    memcpy(&newObj, obj, sizeof(Obj));

    // get current status
    obj_cir(&backup, &newObj);

    *el = newObj.any.co_alt;
    *az = newObj.any.co_az;

    bool isUp = newObj.pl.co_alt > 0;

    riset->rs_tranaz = 0;
    riset->rs_tranalt = 0;
    riset->rs_trantm = 0;

    if (((up && isUp) || (!up && !isUp)) && newObj.o_type != EARTHSAT)
    {
        if (FindAlt0(&backup, &newObj, step, limit, false, !up, up ? &riset->rs_riseaz : &riset->rs_setaz, up ? &riset->rs_risetm : &riset->rs_settm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0 &&
            FindAlt0(&backup, &newObj, step, limit, true, up, up ? &riset->rs_setaz : &riset->rs_riseaz, up ? &riset->rs_settm : &riset->rs_risetm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
        {
            return 0;
        }
    } else {
        if (FindAlt0(&backup, &newObj, step, limit, true, !up, up ? &riset->rs_riseaz : &riset->rs_setaz, up ? &riset->rs_risetm : &riset->rs_settm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
        {
            riset->rs_tranaz = 0;
            riset->rs_tranalt = 0;
            riset->rs_trantm = 0;
            // set time is always behind rise time
            backup.n_mjd = up ? riset->rs_risetm : riset->rs_settm;
            if (FindAlt0(&backup, &newObj, step, limit, true, up, up ? &riset->rs_setaz : &riset->rs_riseaz, up ? &riset->rs_settm : &riset->rs_risetm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
            {
                return 0;
            }
        }
    }
    return RS_ERROR;
}

void ConfigureObserver(double longitude, double latitude, double altitude, double seconds_since_epoch, Now *obj)
{
    memset(obj, 0, sizeof(Now));
    obj->n_lng = radian(longitude);
    obj->n_lat = radian(latitude);
    obj->n_elev = altitude / ERAD;
    obj->n_dip = 0;
    obj->n_temp = 15.0;
    obj->n_tz = 0;
    obj->n_mjd = EpochToEphemTime(seconds_since_epoch);
    obj->n_pressure = 1010;
    obj->n_epoch = J2000;
}

double fmod2(double m1, double m2)
{
    return m1 - floor(m1 / m2) * m2;
}

double calc_phase(double x, double antitarget)
{
    Obj moonObj;
    memset(&moonObj, 0, sizeof(Obj));
    moonObj.pl.plo_code = MOON;
    moonObj.any.co_type = PLANET;
    Obj sunObj;
    memset(&sunObj, 0, sizeof(Obj));
    sunObj.pl.plo_code = SUN;
    sunObj.any.co_type = PLANET;
    Now now;
    memset(&now, 0, sizeof(Now));
    now.n_mjd = x;
    now.n_pressure = 1010;
    obj_cir(&now, &sunObj);
    obj_cir(&now, &moonObj);
    double slon, slat, mlon, mlat;
    eq_ecl(now.n_mjd, sunObj.pl.co_gaera, sunObj.pl.co_gaedec, &slat, &slon);
    eq_ecl(now.n_mjd, moonObj.pl.co_gaera, moonObj.pl.co_gaedec, &mlat, &mlon);
    double res = fmod2(mlon - slon - antitarget, 2 * M_PI) - M_PI;
    return res;
}

double CurrentMoonPhase(double seconds_since_epoch)
{
    double k;
    moon_colong(MJD0 + EpochToEphemTime(seconds_since_epoch), 0, 0, 0, &k, 0, 0);
    return k;
}

double FindMoonPhase(double seconds_since_epoch, double motion, double target)
{
    double antitarget = target + M_PI;
    double time = EpochToEphemTime(seconds_since_epoch);
    double res = calc_phase(time, antitarget);
    double angle_to_cover = fmod2(-res, motion);
    double dd = time + 29.53 * angle_to_cover / (2 * M_PI);
    double hour = 1.0 / 24;
    double x0 = dd;
    double x1 = dd + hour;
    double f0 = calc_phase(x0, antitarget);
    double f1 = calc_phase(x1, antitarget);
    while (fabs(x1 - x0) > 1.0 / 24 / 60 && f1 != f0)
    {
        double x2 = x0;
        x0 = x1;
        x1 = x1 + (x1 - x2) / (f0 / f1 - 1);
        f0 = f1;
        f1 = calc_phase(x1, antitarget);
    }
    return EphemToEpochTime(x1);
}

double GetLST(double now, double longitude)
{
    Now _now;
    ConfigureObserver(longitude, 0, 0, now, &_now);

    double lst;

    now_lst(&_now, &lst);

    return lst;
}

void GetRADECRiset(double ra, double dec, double longitude, double latitude, double now, double *riseTime, double *setTime, double *transitTime, int *status, double *az_r, double *az_s, double *az_c, double *az_t, double *el_c, double *el_t)
{
    double r, s;
    riset(radian(ra), radian(dec), radian(latitude), 0, &r, &s, az_r, az_s, status);
    double lst = GetLST(now, longitude);

    hadec_aa(radian(latitude), radian(lst * 15 - ra), radian(dec), el_c, az_c);

    if (s < r)
    {
        r -= 23.93446959189;
    }
    if (*status == 1 || *status == -1)
    {
        *riseTime = 0;
        *setTime = 0;
    }
    else
    {
        if (lst > s)
        {
            r += 23.93446959189;
            s += 23.93446959189;
        }
        *riseTime = now + (r - lst) * 3600;
        *setTime = now + (s - lst) * 3600;
        *transitTime = (*riseTime + *setTime) / 2.0;

        double lst = GetLST(*transitTime, longitude);
        hadec_aa(radian(latitude), radian(lst * 15 - ra), radian(dec), el_t, az_t);
    }
}

int GetSatellitePosition(const char* line0, const char* line1, const char* line2, double longitude, double latitude, double altitude, double seconds_since_epoch, double* el, double* az)
{
    Obj satillite;
    /* Construct the Satellite */
    if (db_tle((char*)line0, (char*)line1, (char*)line2, &satillite) != 0)
        return 0;

    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, seconds_since_epoch, &now);

    /* Current Position */
    obj_earthsat(&now, &satillite);

    if (el)
        *el = satillite.any.co_alt;
    if (az)
        *az = satillite.any.co_az;
    return 1;
}

int GetSatelliteStatus(const char* line0, const char* line1, const char* line2, double seconds_since_epoch, double* sublng, double* sublat, double* elevation)
{
    Obj satillite;
    /* Construct the Satellite */
    if (db_tle((char*)line0, (char*)line1, (char*)line2, &satillite) != 0)
        return 0;

    /* Construct the observer */
    Now now;
    ConfigureObserver(0, 0, 0, seconds_since_epoch, &now);

    /* Current Position */
    obj_earthsat(&now, &satillite);

    if (sublng)
        *sublng = satillite.s_sublng / M_PI * 180;
    if (sublat)
        *sublat = satillite.s_sublat / M_PI * 180;
    if (elevation)
        *elevation = satillite.s_elev;
    return 1;
}

int GetNextSatellitePass(const char* line0, const char* line1, const char* line2, double seconds_since_epoch, double longitude, double latitude, double altitude, RiseSet* riset, RiseSet* visibleRiset, double* visibleRiseAlt, double* visibleSetAlt)
{
    double azimuth, elevation;
    Obj satillite;
    /* Construct the Satellite */
    if (db_tle((char*)line0, (char*)line1, (char*)line2, &satillite) != 0)
        return 0;

    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, seconds_since_epoch, &now);
    int result = GetModifiedRisetS(&now, &satillite, 1.0 / 1440 / 6, 10, riset, &elevation, &azimuth, true);
    if (result == 0 && visibleRiset) {
        double step = 1.0 / 1440 / 6;
        double time = riset->rs_risetm;
        bool visibleFound = false;
        for (; time < riset->rs_settm; time += step)
        {
            Now current;
            ConfigureObserver(longitude, latitude, altitude, EphemToEpochTime(time), &current);
            Obj sunObj;
            sunObj.pl.plo_code = SUN;
            sunObj.any.co_type = PLANET;
            obj_cir(&current, &sunObj);
            double sunAltitude = sunObj.pl.co_alt / M_PI * 180;
            bool visible = sunAltitude < -6 && sunAltitude > -30;
            if (visible)
            {
                obj_cir(&current, &satillite);
                visible = (satillite.es.co_alt / M_PI * 180) >= 10.0 && !satillite.s_eclipsed;
            }
            if (!visibleFound)
            {
                if (visible)
                {
                    visibleFound = true;
                    visibleRiset->rs_risetm = time;
                    visibleRiset->rs_riseaz = satillite.es.co_az;
                    if (visibleRiseAlt)
                        *visibleRiseAlt = satillite.es.co_alt;
                }
            }
            else
            {
                if (!visible)
                {
                    break;
                }
            }
        }
        if (visibleFound) {
            visibleRiset->rs_settm = time;
            visibleRiset->rs_setaz = satillite.es.co_az;
            if (visibleSetAlt)
                *visibleSetAlt = satillite.es.co_alt;
            visibleRiset->rs_flags = 0;
        } else {
            visibleRiset->rs_flags = RS_ERROR;
        }
    }
    return result;
}

int GetSunStatus(double altitude, bool isGoingUp, bool hasUpAndDown)
{
    int state = 0;

    if (altitude <= -18)
        state = STATUS_NIGHT;
    else if (altitude <= -12) {
        if (hasUpAndDown)
            state = STATUS_ASTRONOMICAL_UNKNOWN;
        else if (isGoingUp)
            state = STATUS_ASTRONOMICAL_TWILIGHT;
        else
            state = STATUS_ASTRONOMICAL_DUSK;
    } else if (altitude <= -6) {
        if (hasUpAndDown)
            state = STATUS_NAUTICAL_UNKNOWN;
        else if (isGoingUp)
            state = STATUS_NAUTICAL_TWILIGHT;
        else
            state = STATUS_NAUTICAL_DUSK;
    } else if (altitude <= -4) {
        if (hasUpAndDown)
            state = STATUS_BLUE_HOUR_UNKNOWN;
        else if (isGoingUp)
            state = STATUS_BLUE_HOUR_TWILIGHT;
        else
            state = STATUS_BLUE_HOUR_DUSK;
    } else if (altitude <= 0) {
        if (hasUpAndDown)
            state = STATUS_CIVIL_UNKNOWN;
        else if (isGoingUp)
            state = STATUS_CIVIL_TWILIGHT;
        else
            state = STATUS_CIVIL_DUSK;
    } else if (altitude <= 6) {
        if (hasUpAndDown)
            state = STATUS_GOLDEN_HOUR_UNKNOWN;
        else if (isGoingUp)
            state = STATUS_GOLDEN_HOUR_TWILIGHT;
        else
            state = STATUS_GOLDEN_HOUR_DUSK;
    } else {
        state = STATUS_DAY;
    }
    return state;
}


std::vector<TimePeriod> GetSunDetails(double longitude, double latitude, double altitude, double startTime, double endTime)
{
    std::vector<TimePeriod> periods;
    double currentTime = startTime;
    double step = 20;

    int currentStatus = 0;
    double currentStartTime = 0;

    double currentAltitude = 0.0;
    bool isCurrentAltitudeValid = false;
    bool isGoingUp = false;
    bool isGoingUpValid = false;
    bool hasUpAndDown = false;

    currentTime -= step;
    while (currentTime <= endTime)
    {
        Now current;
        ConfigureObserver(longitude, latitude, altitude, currentTime, &current);

        Obj sunObj;
        sunObj.pl.plo_code = SUN;
        sunObj.any.co_type = PLANET;
        obj_cir(&current, &sunObj);
        double sunAltitude = sunObj.pl.co_alt / M_PI * 180;
        if (!isCurrentAltitudeValid) {
            currentAltitude = sunAltitude;
            isCurrentAltitudeValid = true;
        } else {
            bool isCurrentlyGoingUp = sunAltitude >= currentAltitude;
            if (!isGoingUpValid) {
                isGoingUp = isCurrentlyGoingUp;
                isGoingUpValid = true;
            } else {
                if (!hasUpAndDown && (isGoingUp != isCurrentlyGoingUp)) {
                    hasUpAndDown = true;
                }
                isGoingUp = isCurrentlyGoingUp;
            }
            currentAltitude = sunAltitude;
        }
        if (isGoingUpValid)
        {
            int status = GetSunStatus(sunAltitude, isGoingUp, hasUpAndDown);
            if (status != currentStatus)
            {
                if (currentStatus)
                {
                    TimePeriod period {currentStartTime - startTime,currentTime - startTime, currentStatus};
                    periods.push_back(period);
                }
                currentStartTime = currentTime;
                currentStatus = status;
                hasUpAndDown = false;
            }
        }
        currentTime += step;
    }

    if (currentStatus)
    {
        TimePeriod period {currentStartTime - startTime,endTime - startTime, currentStatus};
        periods.push_back(period);
    }
    return periods;
}

astro::Date::Date() : Date(0, 0, 0)
{
}

astro::Date::Date(int Y, int M, int D) :
    year(Y),
    month(M),
    day(D),
    hour(0),
    minute(0),
    seconds(0.0),
    wday(0),
    utc_offset(0),
    tzname("UTC")
{
}

astro::Date::Date(double jd)
{
    auto a = (int64_t) floor(jd + 0.5);
    wday = (a + 1) % 7;
    double c;
    if (a < 2299161)
    {
        c = (double) (a + 1524);
    }
    else
    {
        double b = (double) ((int64_t) floor((a - 1867216.25) / 36524.25));
        c = a + b - (int64_t) floor(b / 4) + 1525;
    }

    auto d = (int64_t) floor((c - 122.1) / 365.25);
    auto e = (int64_t) floor(365.25 * d);
    auto f = (int64_t) floor((c - e) / 30.6001);

    double dday = c - e - (int64_t) floor(30.6001 * f) + ((jd + 0.5) - a);

    // This following used to be 14.0, but gcc was computing it incorrectly, so
    // it was changed to 14
    month = (int) (f - 1 - 12 * (int64_t) (f / 14));
    year = (int) (d - 4715 - (int64_t) ((7.0 + month) / 10.0));
    day = (int) dday;

    double dhour = (dday - day) * 24;
    hour = (int) dhour;

    double dminute = (dhour - hour) * 60;
    minute = (int) dminute;

    seconds = (dminute - minute) * 60;
    utc_offset = 0;
    tzname = "UTC";
}

const char* astro::Date::toCStr(Format format) const
{
    static char date[255];

    if (format == ISO8601)
    {
        snprintf(date, sizeof(date), "%04d-%02d-%02dT%02d:%02d:%08.5fZ",
                 year, month, day, hour, minute, seconds);
        return date;
    }
    // MinGW's libraries don't have the tm_gmtoff and tm_zone fields for
    // struct tm.
#if defined(__GNUC__) && !defined(_WIN32)
    struct tm cal_time {};
    cal_time.tm_year = year-1900;
    cal_time.tm_mon = month-1;
    cal_time.tm_mday = day;
    cal_time.tm_hour = hour;
    cal_time.tm_min = minute;
    cal_time.tm_sec = (int)seconds;
    cal_time.tm_wday = wday;
    cal_time.tm_gmtoff = utc_offset;
#if defined(__APPLE__) || defined(__FreeBSD__)
    // tm_zone is a non-const string field on the Mac and FreeBSD (why?)
    cal_time.tm_zone = const_cast<char*>(tzname.c_str());
#else
    cal_time.tm_zone = tzname.c_str();
#endif

    const char* strftime_format;
    switch(format)
    {
    case Locale:
        strftime_format = "%c";
        break;
    case TZName:
        strftime_format = "%Y %b %d %H:%M:%S %Z";
        break;
    default:
        strftime_format = "%Y %b %d %H:%M:%S %z";
        break;
    }

    strftime(date, sizeof(date), strftime_format, &cal_time);
#else
    switch(format)
    {
    case Locale:
    case TZName:
        snprintf(date, sizeof(date), "%04d %s %02d %02d:%02d:%02d %s",
                 year, _(MonthAbbrList[month-1]), day,
                 hour, minute, (int)seconds, tzname.c_str());
        break;
    case UTCOffset:
        {
            int sign = utc_offset < 0 ? -1:1;
            int h_offset = sign * utc_offset / 3600;
            int m_offset = (sign * utc_offset - h_offset * 3600) / 60;
            snprintf(date, sizeof(date), "%04d %s %02d %02d:%02d:%02d %c%02d%02d",
                    year, _(MonthAbbrList[month-1]), day,
                    hour, minute, (int)seconds, (sign==1?'+':'-'), h_offset, m_offset);
        }
        break;
    }
#endif

    return date;

}

// Convert a calendar date to a Julian date
astro::Date::operator double() const
{
    int y = year, m = month;
    if (month <= 2)
    {
        y = year - 1;
        m = month + 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15))))
    {
        B = y / 400 - y / 100;
    }

    return (floor(365.25 * y) +
            floor(30.6001 * (m + 1)) + B + 1720996.5 +
            day + hour / HOURS_PER_DAY + minute / MINUTES_PER_DAY + seconds / SECONDS_PER_DAY);
}

astro::Date
astro::Date::systemDate()
{
    time_t t = time(nullptr);
    struct tm *gmt = gmtime(&t);
    astro::Date d;
    d.year = gmt->tm_year + 1900;
    d.month = gmt->tm_mon + 1;
    d.day = gmt->tm_mday;
    d.hour = gmt->tm_hour;
    d.minute = gmt->tm_min;
    d.seconds = (int) gmt->tm_sec;

    return d;
}


ostream& operator<<(ostream& s, const astro::Date& d)
{
    s << d.toCStr();
    return s;
}
