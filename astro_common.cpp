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
