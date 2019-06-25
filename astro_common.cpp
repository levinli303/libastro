//
// Created by 李林峰 on 2019-06-25.
//

#include "astro_common.h"
#include <cstring>
#include <cmath>

#define EPHEM_SECONDS_DIFFERENCE   2209032000

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

int FindAlt0(Now *now, Obj *obj, double step, double limit, int forward, int go_down, double *az, double *jd)
{
    double orig = now->n_mjd;
    double current = orig;

    Obj backup;
    memcpy(&backup, obj, sizeof(Obj));

    obj_cir(now, &backup);

    double prev_az = backup.pl.co_az;
    double prev_alt = backup.pl.co_alt;
    double prev_time = current;

    memcpy(&backup, obj, sizeof(Obj));

    go_down = forward ? go_down : !go_down;

    for (;;)
    {
        now->n_mjd = current;
        obj_cir(now, &backup);

        double curr_alt = backup.pl.co_alt;
        double curr_az = backup.pl.co_az;

        memcpy(&backup, obj, sizeof(Obj));

        if (go_down)
        {
            if (prev_alt >= 0 && curr_alt <= 0)
            {
                *az = (prev_az + curr_az) / 2;
                *jd = (prev_time + current) / 2;
                now->n_mjd = orig;
                return 0;
            }
        }
        else
        {
            if (prev_alt <= 0 && curr_alt >= 0)
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

int GetModifiedRiset(Now *now, int index, RiseSet *riset)
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

    int isUp = obj.pl.co_alt > 0;

    memcpy(&obj, &origObj, sizeof(Obj));

    if (isUp) {
        if (FindAlt0(&backup, &obj, step, limit, false, false, &riset->rs_riseaz, &riset->rs_risetm) == 0 &&
            FindAlt0(&backup, &obj, step, limit, true, true, &riset->rs_settm, &riset->rs_settm) == 0)
        {
            return 0;
        }
    } else {
        if (FindAlt0(&backup, &obj, step, limit, true, false, &riset->rs_riseaz, &riset->rs_risetm) == 0 &&
            FindAlt0(&backup, &obj, step, limit, true, true, &riset->rs_settm, &riset->rs_settm) == 0)
        {
            return 0;
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
