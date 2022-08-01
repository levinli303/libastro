#include <jni.h>
#include <cmath>
#include <cstring>

#include "astro_common.h"

namespace {
    jlong getTime(JNIEnv *env, jobject dateObject) {
        jclass dateCls = env->FindClass("java/util/Date");
        jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");
        jlong origTime = env->CallLongMethod(dateObject, getTimeMethod);
        return origTime;
    }

    jobject createDateObject(JNIEnv *env, jlong time) {
        jclass dateCls = env->FindClass("java/util/Date");
        jmethodID dateInitMethod =  env->GetMethodID(dateCls, "<init>", "(J)V");
        return env->NewObject(dateCls, dateInitMethod, time);
    }
}

jobject getRisetAtIndex(JNIEnv *env,
                           jdouble longitude, jdouble latitude, jdouble altitude,
                           jobject time, jint index, jboolean up) {

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Ljava/lang/String;)V");

    jlong origTime = getTime(env, time);

    Now now;
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    RiseSet riset;
    jobject rise = nullptr;
    jobject set = nullptr;
    jobject peak = nullptr;
    double el, az;
    int result = GetModifiedRiset(&now, (int)index, &riset, &el, &az, up == JNI_TRUE);
    jobject current = env->NewObject(posCls, posInitMethod, (jdouble)el, (jdouble)az, origTime);
    if (result == 0) {
        rise = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_riseaz, (jlong)(EphemToEpochTime(riset.rs_risetm) * 1000));
        set = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_setaz, (jlong)(EphemToEpochTime(riset.rs_settm) * 1000));
        peak = env->NewObject(posCls, posInitMethod, (jdouble)riset.rs_tranalt, (jdouble)riset.rs_tranaz, (jlong)(EphemToEpochTime(riset.rs_trantm) * 1000));
    }

    return env->NewObject(risetCls, risetInitMethod, rise, set, peak, current, env->NewStringUTF(GetStarName(index)));
}


jobject getSunAltTime(JNIEnv *env,
                      jdouble longitude, jdouble latitude, jdouble altitude,
                      jboolean go_down,
                      jobject time,
                      jdouble x)
{
    const double step = 1.0 / 1440;
    const double limit = 2;
    Now now;
    jlong origTime = getTime(env, time);
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    double jd = 0;
    if (FindAltXSun(&now, step, limit, 1, (go_down == JNI_TRUE) ? 1 : 0, &jd, x))
        return nullptr;
    return createDateObject(env, (jlong)(EphemToEpochTime(jd) * 1000));
}

jobject getRiset(JNIEnv *env,
                 jdouble longitude, jdouble latitude,
                 jdouble altitude, jobject time) {
    jclass sunMoonCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunMoonRiset");
    jmethodID sunMoonInitMethod = env->GetMethodID(sunMoonCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;)V");

    jobject sunriset = getRisetAtIndex(env, longitude, latitude, altitude, time, SUN, JNI_TRUE);
    jobject moonriset = getRisetAtIndex(env, longitude, latitude, altitude, time, MOON, JNI_TRUE);

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
        env->CallBooleanMethod(result, arrayListAdd, getRisetAtIndex(env, longitude, latitude, altitude, time, i, JNI_TRUE));
    return result;
}

jobject getLunarPhase(JNIEnv *env, jobject time) {
    jclass lpCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/LunarPhase");
    jmethodID lpInitMethod = env->GetMethodID(lpCls, "<init>", "(JJLjava/lang/String;DZ)V");

    jlong mi = getTime(env, time);

    double now = (double)mi / 1000;
    double pn = FindMoonPhase(now, M_PI * -2, 0);
    double nn = FindMoonPhase(now, M_PI * 2, 0);
    double nf = FindMoonPhase(now, M_PI * 2, M_PI);
    double pnn = FindMoonPhase(pn, M_PI * 2, M_PI);
    double phase = CurrentMoonPhase(now);
    bool isFirstHalf = now <= pnn;

    const char *_name = nullptr;

    if (phase <= 0.01) {
        _name = "New Moon";
    } else if (phase < 0.49) {
        if (isFirstHalf) {
            _name = "Waxing Crescent";
        } else {
            _name = "Waning Crescent";
        }
    } else if (phase <= 0.51) {
        if (isFirstHalf) {
            _name = "First Quarter";
        } else {
            _name = "Last Quarter";
        }
    } else if (phase < 0.99) {
        if (isFirstHalf) {
            _name = "Waxing Gibbous";
        } else {
            _name = "Waning Gibbous";
        }
    } else {
        _name = "Full Moon";
    }

    return env->NewObject(lpCls, lpInitMethod, (jlong)(nn * 1000), (jlong)(nf * 1000), env->NewStringUTF(_name), phase, isFirstHalf ? JNI_TRUE : JNI_FALSE);
}

jobject getStarRiset(JNIEnv *env, jdouble ra, jdouble dec, jdouble ra_pm, jdouble dec_pm, jdouble longitude, jdouble latitude, jdouble altitude, jobject time)
{
    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/StarRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;)V");

    jlong origTime = getTime(env, time);

    double riseTime, setTime, transitTime;
    int status;
    double az_r, az_s, az_c, az_t, el_c, el_t;
    GetRADECRiset(ra, dec, longitude, latitude, origTime / 1000.0, &riseTime, &setTime, &transitTime, &status, &az_r, &az_s, &az_c, &az_t, &el_c, &el_t);

    jobject rise = nullptr;
    jobject set = nullptr;
    jobject peak = nullptr;
    jobject current = env->NewObject(posCls, posInitMethod, (jdouble)el_c, (jdouble)az_c, origTime);
    if (status == 0)
    {
        rise = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)az_r, (jlong)(riseTime * 1000));
        set = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)az_s, (jlong)(setTime * 1000));
        peak = env->NewObject(posCls, posInitMethod, (jdouble)el_t, (jdouble)az_t, (jlong)(transitTime * 1000));
    }

    return env->NewObject(risetCls, risetInitMethod, rise, set, peak, current);
}

jdouble getLST(JNIEnv *env, jdouble longitude, jobject time)
{
    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");
    jlong origTime = env->CallLongMethod(time, getTimeMethod);

    return (jdouble)GetLST(origTime / 1000.0, longitude);
}

jobject getSatelliteStatus(JNIEnv *env,
                           jstring line0, jstring line1,
                           jstring line2, jobject time)
{
    jlong origTime = getTime(env, time);
    const char *str0 = env->GetStringUTFChars(line0, nullptr);
    const char *str1 = env->GetStringUTFChars(line1, nullptr);
    const char *str2 = env->GetStringUTFChars(line2, nullptr);
    double sublng, sublat, elevation;
    int result = GetSatelliteStatus(str0, str1, str2, origTime / 1000.0, &sublng, &sublat, &elevation);
    env->ReleaseStringUTFChars(line0, str0);
    env->ReleaseStringUTFChars(line1, str1);
    env->ReleaseStringUTFChars(line2, str2);
    if (!result)
        return nullptr;

    jclass statusCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SatelliteStatus");
    jmethodID statusInitMethod = env->GetMethodID(statusCls, "<init>", "(DDD)V");
    return env->NewObject(statusCls, statusInitMethod, (jdouble)sublng, (jdouble)sublat, (jdouble)elevation);
}

jobject getSatelliteNextRiset(JNIEnv *env,
                              jstring line0, jstring line1,
                              jstring line2, jobject time,
                              jdouble longitude,
                              jdouble latitude,
                              jdouble altitude)
{
    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SatellitePass");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>",
                                                 "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;)V");

    jlong origTime = getTime(env, time);
    const char *str0 = env->GetStringUTFChars(line0, nullptr);
    const char *str1 = env->GetStringUTFChars(line1, nullptr);
    const char *str2 = env->GetStringUTFChars(line2, nullptr);
    RiseSet riset;
    RiseSet visibleRiset;
    double visibleRiseAlt, visibleSetAlt;
    int result = GetNextSatellitePass(str0, str1, str2, origTime / 1000.0, (double)longitude, (double)latitude, (double)altitude, &riset, &visibleRiset, &visibleRiseAlt, &visibleSetAlt);
    env->ReleaseStringUTFChars(line0, str0);
    env->ReleaseStringUTFChars(line1, str1);
    env->ReleaseStringUTFChars(line2, str2);
    if (result != 0)
        return nullptr;
    jobject rise = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_riseaz, (jlong)(EphemToEpochTime(riset.rs_risetm) * 1000));
    jobject set = env->NewObject(posCls, posInitMethod, (jdouble)0, (jdouble)riset.rs_setaz, (jlong)(EphemToEpochTime(riset.rs_settm) * 1000));
    jobject peak = env->NewObject(posCls, posInitMethod, (jdouble)riset.rs_tranalt, (jdouble)riset.rs_tranaz, (jlong)(EphemToEpochTime(riset.rs_trantm) * 1000));
    jobject visibleRise = nullptr;
    jobject visibleSet = nullptr;
    if (visibleRiset.rs_flags == 0)
    {
        visibleRise = env->NewObject(posCls, posInitMethod, (jdouble)visibleRiseAlt, (jdouble)visibleRiset.rs_riseaz, (jlong)(EphemToEpochTime(visibleRiset.rs_risetm) * 1000));
        visibleSet = env->NewObject(posCls, posInitMethod, (jdouble)visibleSetAlt, (jdouble)visibleRiset.rs_setaz, (jlong)(EphemToEpochTime(visibleRiset.rs_settm) * 1000));
    }
    return env->NewObject(risetCls, risetInitMethod, rise, set, peak, visibleRise, visibleSet);
}

jobject getStarPosition(JNIEnv *env, jdouble ra, jdouble dec,
                        jdouble ra_pm, jdouble dec_pm,
                        jobject time,
                        jdouble longitude, jdouble latitude,
                        jdouble altitude)
{
    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");

    Now now;
    jlong origTime = getTime(env, time);
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    Obj obj;
    obj.o_type = FIXED;
    obj.f_RA = (float)radian(ra);
    obj.f_dec = (float)radian(dec);
    obj.f_epoch = J2000;
    obj.f_pmRA = (float)(ra_pm / 1000 / 3600 / 180 * M_PI / 365.25);
    obj.f_pmdec = (float)(dec_pm / 1000 / 3600 / 180 * M_PI / 365.25);
    obj_cir(&now, &obj);

    return env->NewObject(posCls, posInitMethod, (jdouble)obj.f.co_alt, (jdouble)obj.f.co_az, origTime);
}

jobject getSolarSystemObjectPosition(JNIEnv *env, jint index, jobject time, jdouble longitude, jdouble latitude, jdouble altitude)
{
    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");

    Now now;
    jlong origTime = getTime(env, time);
    ConfigureObserver(longitude, latitude, altitude, (double)origTime / 1000, &now);

    Obj *objs;
    getBuiltInObjs(&objs);

    Obj obj = objs[index];
    obj_cir(&now, &obj);

    return env->NewObject(posCls, posInitMethod, (jdouble)obj.any.co_alt, (jdouble)obj.any.co_az, origTime);
}

jobject getSatellitePosition(JNIEnv *env, jstring line0, jstring line1, jstring line2,  jobject time,
                             jdouble longitude, jdouble latitude,
                             jdouble altitude)
{
    jlong origTime = getTime(env, time);
    const char *str0 = env->GetStringUTFChars(line0, nullptr);
    const char *str1 = env->GetStringUTFChars(line1, nullptr);
    const char *str2 = env->GetStringUTFChars(line2, nullptr);
    double el, az;
    int result = GetSatellitePosition(str0, str1, str2, longitude, latitude, altitude, origTime / 1000.0, &el, &az);
    env->ReleaseStringUTFChars(line0, str0);
    env->ReleaseStringUTFChars(line1, str1);
    env->ReleaseStringUTFChars(line2, str2);
    if (!result)
        return nullptr;

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    return env->NewObject(posCls, posInitMethod, (jdouble)el, (jdouble)az, origTime);
}

jobject getSunTimes(JNIEnv *env,
                    jdouble longitude, jdouble latitude,
                    jdouble altitude, jobject start_time,
                    jobject end_time)
{
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID construct = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jclass stCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunTime");
    jmethodID stInitMethod = env->GetMethodID(stCls, "<init>", "(DDI)V");

    auto periods = GetSunDetails(longitude, latitude, altitude, getTime(env, start_time) / 1000.0f, getTime(env, end_time) / 1000.0f);

    jobject result = env->NewObject(arrayListClass, construct, (jint)(periods.size()));
    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    for (auto period : periods)
    {
        env->CallBooleanMethod(result, arrayListAdd, env->NewObject(stCls, stInitMethod, (jdouble)(period.start), (jdouble)(period.end), (jint)period.status));
    }
    return result;
}

jobject getSolarSystemNextRiset(JNIEnv *env, jint index, jobject time,
                                jdouble longitude,
                                jdouble latitude,
                                jdouble altitude,
                                jboolean up)
{
    return getRisetAtIndex(env, longitude, latitude, altitude, time, index, up);
}