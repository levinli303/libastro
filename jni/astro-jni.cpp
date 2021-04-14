#include <jni.h>
#include <cmath>
#include <cstring>

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
    if (GetModifiedRiset(&now, SUN, &riset, nullptr) == 0) {
        jobject rise = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
        jobject set = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
        sunriset = env->NewObject(risetCls, risetInitMethod, rise, set, nullptr, env->NewStringUTF("Sun"));
    }

    if (GetModifiedRiset(&now, MOON, &riset, nullptr) == 0) {
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
        if (GetModifiedRiset(&now, i, &riset, nullptr) == 0)
        {
            jobject rise = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
            jobject set = env->NewObject(posCls, posInitMethod, (double)0, riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
            jobject riset = env->NewObject(risetCls, risetInitMethod, rise, set, nullptr, env->NewStringUTF(GetStarName(i)));
            env->CallBooleanMethod(result, arrayListAdd, riset);
        }
    }
    return result;
}

jobject getLunarPhase(JNIEnv *env, jobject time) {
    jclass lpCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/LunarPhase");
    jmethodID lpInitMethod = env->GetMethodID(lpCls, "<init>", "(JJLjava/lang/String;D)V");
    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jlong mi = env->CallLongMethod(time, getTimeMethod);

    double pn = FindMoonPhase((double)mi / 1000, M_PI * -2, 0);
    double nn = FindMoonPhase((double)mi / 1000, M_PI * 2, 0);
    double nf = FindMoonPhase((double)mi / 1000, M_PI * 2, M_PI);
    double phase = ((double)mi / 1000 - pn) / (nn - pn);

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

    return env->NewObject(lpCls, lpInitMethod, (jlong)(nn * 1000), (jlong)(nf * 1000), env->NewStringUTF(pcDesc), phase);
}

jobject getStarRiset(JNIEnv *env, jdouble ra, jdouble dec, jdouble longitude, jdouble latitude, jobject time)
{
    jclass srCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/StarRiset");
    jmethodID srInitMethod = env->GetMethodID(srCls, "<init>", "(IJJZ)V");
    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jlong mi = env->CallLongMethod(time, getTimeMethod);

    double riseTime, setTime;
    int status;
    bool up;
    GetRADECRiset(ra, dec, longitude, latitude, mi / 1000.0, &riseTime, &setTime, &status, &up);

    return env->NewObject(srCls, srInitMethod, (jint)status, (jlong)(riseTime * 1000), (jlong)(setTime * 1000), up ? JNI_TRUE : JNI_FALSE);
}