#include <jni.h>
#include <cmath>
#include <cstring>
#include "astro.h"
#include "astro_common.h"

jobject getRiset(JNIEnv *env,
                 jdouble longitude, jdouble latitude,
                 jdouble altitude, jobject time) {
    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID updatePosMehod = env->GetMethodID(posCls, "update", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Ljava/lang/String;)V");

    jclass sunMoonCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunMoonRiset");
    jmethodID sunMoonInitMethod = env->GetMethodID(sunMoonCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;)V");
    jlong origTime = env->CallLongMethod(time, getTimeMethod);

    Now now;
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    jobject sunriset = nullptr;
    jobject moonriset = nullptr;

    RiseSet riset;
    if (GetModifiedRiset(&now, SUN, &riset) == 0) {
        jobject rise = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
        jobject set = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
        sunriset = env->NewObject(risetCls, risetInitMethod, rise, set, nullptr, env->NewStringUTF("Sun"));
    }

    if (GetModifiedRiset(&now, MOON, &riset) == 0) {
        jobject rise = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
        jobject set = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
        moonriset = env->NewObject(risetCls, risetInitMethod, rise, set, nullptr, env->NewStringUTF("Moon"));
    }

    return env->NewObject(sunMoonCls, sunMoonInitMethod, sunriset, moonriset);
}


jobject getAllRiset(JNIEnv *env,
                    jdouble longitude, jdouble latitude, jdouble altitude,
                    jobject time) {

    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID updatePosMehod = env->GetMethodID(posCls, "update", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Ljava/lang/String;)V");

    jlong origTime = env->CallLongMethod(time, getTimeMethod);

    Now now;
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID construct = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject result = env->NewObject(arrayListClass, construct, MOON - MERCURY + 1);
    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    RiseSet riset;
    for (int i = MERCURY; i <= MOON; ++i)
    {
        if (GetModifiedRiset(&now, i, &riset) == 0)
        {
            jobject rise = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
            jobject set = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
            jobject riset = env->NewObject(risetCls, risetInitMethod, rise, set, nullptr, env->NewStringUTF(GetStarName(i)));
            env->CallBooleanMethod(result, arrayListAdd, riset);
        }
    }
    return result;
}

jdouble modifiedJulianDate(JNIEnv *env, jobject time) {
    jclass tzCls = env->FindClass("java/util/TimeZone");
    jmethodID tzInitMethod = env->GetStaticMethodID(tzCls, "getTimeZone", "(Ljava/lang/String;)Ljava/util/TimeZone;");
    jobject timezone = env->CallStaticObjectMethod(tzCls, tzInitMethod, env->NewStringUTF("GMT"));

    jclass calCls = env->FindClass("java/util/Calendar");
    jmethodID getCalMethod = env->GetStaticMethodID(calCls, "getInstance", "(Ljava/util/TimeZone;)Ljava/util/Calendar;");
    jobject calendar = env->CallStaticObjectMethod(calCls, getCalMethod, timezone);

    jmethodID setTimeMethod = env->GetMethodID(calCls, "setTime", "(Ljava/util/Date;)V");
    env->CallVoidMethod(calendar, setTimeMethod, time);

    jmethodID getCompoMethod = env->GetMethodID(calCls, "get", "(I)I");
    int year = env->CallIntMethod(calendar, getCompoMethod, 1);
    int month = env->CallIntMethod(calendar, getCompoMethod, 2) + 1;
    int day = env->CallIntMethod(calendar, getCompoMethod, 5);
    int hour = env->CallIntMethod(calendar, getCompoMethod, 11);
    int minute = env->CallIntMethod(calendar, getCompoMethod, 12);
    int second = env->CallIntMethod(calendar, getCompoMethod, 13);

    double x;
    cal_mjd(month, day + (hour * 3600 + minute * 60) / 86400.0, year, &x);
    return x;
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

jlong findMoonPhase(JNIEnv *env, jobject date, jlong mi, jdouble motion, jdouble target) {
    double antitarget = target + M_PI;
    double time = modifiedJulianDate(env, date);
    double res = calc_phase(time, antitarget);
    double angle_to_cover = fmod2(-res, motion);
    double dd = time + 29.53 * angle_to_cover / (2 * M_PI);
    double hour = 1.0 / 24;
    double x0 = dd;
    double x1 = dd + hour;
    double f0 = calc_phase(x0, antitarget);
    double f1 = calc_phase(x1, antitarget);
    while (fabs(x1 - x0) > 1.0 / 24 / 60 && f1 != f0) {
        double x2 = x0;
        x0 = x1;
        x1 = x1 + (x1 - x2) / (f0 / f1 - 1);
        f0 = f1;
        f1 = calc_phase(x1, antitarget);
    }
    return mi + (x1 - time) * 86400000;
}

jobject getLunarPhase(JNIEnv *env, jobject time) {
    jclass lpCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/LunarPhase");
    jmethodID lpInitMethod = env->GetMethodID(lpCls, "<init>", "(JJLjava/lang/String;D)V");
    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jlong mi = env->CallLongMethod(time, getTimeMethod);

    jlong pn = findMoonPhase(env, time, mi, M_PI * -2, 0);
    jlong nn = findMoonPhase(env, time, mi, M_PI * 2, 0);
    jlong nf = findMoonPhase(env, time, mi, M_PI * 2, M_PI);
    double phase = (mi - pn) / (double)(nn - pn);

    const char *pcDesc = NULL;

    if (phase <= 0.01 || phase >= 0.99) {
        pcDesc = "New Moon";
    } else if (phase < 0.24) {
        pcDesc = "Waxing Crescent";
    } else if (phase <= 0.26) {
        pcDesc = "First Quarter";
    } else if (phase < 0.49) {
        pcDesc = "Waxing Gibbous";
    } else if (phase <= 0.51) {
        pcDesc = "Full Moon";
    } else if (phase < 0.74) {
        pcDesc = "Waning Crescent";
    } else if (phase < 0.76) {
        pcDesc = "Last Quarter";
    } else if (phase < 0.99) {
        pcDesc = "Waning Gibbous";
    }

    return env->NewObject(lpCls, lpInitMethod, nn, nf, env->NewStringUTF(pcDesc), phase);
}
