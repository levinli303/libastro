//
// Created by 李林峰 on 2019-06-25.
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

int GetModifiedRiset(Now *now, int index, RiseSet *riset, double *el, double *az)
{
    const double step = 1.0 / 1440;
    const double limit = 1;

    Now backup;
    memcpy(&backup, now, sizeof(Now));

    Obj *objs;
    getBuiltInObjs(&objs);

    Obj origObj = objs[index];
    Obj obj;

    memcpy(&obj, &origObj, sizeof(Obj));

    // get current status
    obj_cir(&backup, &obj);

    *el = obj.pl.co_alt;
    *az = obj.pl.co_az;

    bool isUp = obj.pl.co_alt > 0;
    memcpy(&obj, &origObj, sizeof(Obj));

    riset->rs_tranaz = 0;
    riset->rs_tranalt = 0;
    riset->rs_trantm = 0;

    if (isUp)
    {
        if (FindAlt0(&backup, &obj, step, limit, false, false, &riset->rs_riseaz, &riset->rs_risetm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0 &&
            FindAlt0(&backup, &obj, step, limit, true, true, &riset->rs_setaz, &riset->rs_settm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
        {
            return 0;
        }
    } else {
        if (FindAlt0(&backup, &obj, step, limit, true, false, &riset->rs_riseaz, &riset->rs_risetm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
        {
            riset->rs_tranaz = 0;
            riset->rs_tranalt = 0;
            riset->rs_trantm = 0;
            // set time is always behind rise time
            backup.n_mjd = riset->rs_risetm;
            if (FindAlt0(&backup, &obj, step, limit, true, true, &riset->rs_setaz, &riset->rs_settm, &riset->rs_tranaz, &riset->rs_tranalt, &riset->rs_trantm) == 0)
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
