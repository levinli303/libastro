//
// Created by 李林峰 on 2019-06-25.
//

#include "astro_common.h"
#include "astro.h"
#include <cstring>

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

int FindAlt0(Now *now, Obj *obj, double step, double limit, int forward, int go_down, double *az, double *jd)
{
    double orig = now->n_mjd;
    double current = orig;

    obj_cir(now, obj);
    double prev_az = obj->es.co_az;
    double prev_alt = obj->es.co_alt;
    double prev_time = current;

    go_down = forward ? go_down : !go_down;

    for (;forward ? (current < orig + limit) : (current > orig - limit); forward ? current += 1 : current -= 1)
    {
        now->n_mjd = current;
        obj_cir(now, obj);

        double curr_alt = obj->es.co_alt;
        double curr_az = obj->es.co_az;

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

    Obj obj = objs[index];

    // get current status
    obj_cir(&backup, &obj);

    if (obj.es.co_alt > 0) {
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