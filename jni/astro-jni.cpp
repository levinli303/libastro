#include <jni.h>
#include <cmath>
#include <cstring>

#include "astro_common.h"

jobject getRisetAtIndex(JNIEnv *env,
                           jdouble longitude, jdouble latitude, jdouble altitude,
                           jobject time, int index) {

    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;ZLjava/lang/String;)V");

    jlong origTime = env->CallLongMethod(time, getTimeMethod);

    Now now;
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    RiseSet riset;
    bool isUp;
    jobject rise = nullptr;
    jobject set = nullptr;
    jobject peak = nullptr;
    if (GetModifiedRiset(&now, index, &riset, &isUp) == 0) {
        rise = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_riseaz, jlong(EphemToEpochTime(riset.rs_risetm) * 1000));
        set = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_setaz, jlong(EphemToEpochTime(riset.rs_settm) * 1000));
        peak = env->NewObject(posCls, posInitMethod, (jdouble)riset.rs_tranalt, (jdouble)riset.rs_tranaz, jlong(EphemToEpochTime(riset.rs_trantm) * 1000));
    }

    return env->NewObject(risetCls, risetInitMethod, rise, set, peak, isUp ? JNI_TRUE : JNI_FALSE, env->NewStringUTF(GetStarName(index)));
}


jobject getRiset(JNIEnv *env,
                 jdouble longitude, jdouble latitude,
                 jdouble altitude, jobject time) {
    jclass sunMoonCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunMoonRiset");
    jmethodID sunMoonInitMethod = env->GetMethodID(sunMoonCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;)V");

    jobject sunriset = getRisetAtIndex(env, longitude, latitude, altitude, time, SUN);
    jobject moonriset = getRisetAtIndex(env, longitude, latitude, altitude, time, MOON);

    return env->NewObject(sunMoonCls, sunMoonInitMethod, sunriset, moonriset);
}

jobject getAllRiset(JNIEnv *env,
                    jdouble longitude, jdouble latitude, jdouble altitude,
                    jobject time) {
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID construct = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject result = env->NewObject(arrayListClass, construct, (jint)(MOON - MERCURY + 1));
    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    for (int i = MERCURY; i <= MOON; ++i)
        env->CallBooleanMethod(result, arrayListAdd, getRisetAtIndex(env, longitude, latitude, altitude, time, i));
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