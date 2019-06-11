#include <jni.h>
#include <vector>
#include "degree.h"
#include "acoord.h"
#include "planets.h"
#include "astro.h"

#define min(a,b) ((a)<(b))?(a):(b)
#define max(a,b) ((a)>(b))?(a):(b)

jobject getRiset(JNIEnv *env,
                 jdouble longitude, jdouble latitude,
                 jdouble altitude, jobject time) {
    astro::Degree lt(latitude * 3600);
    astro::Degree lg(longitude * 3600);
    double sea = altitude;

    astro::AstroCoordinate acoord;
    astro::Planets pl;
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, sea);

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

    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID updatePosMehod = env->GetMethodID(posCls, "update", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Ljava/lang/String;)V");

    jclass sunMoonCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunMoonRiset");
    jmethodID sunMoonInitMethod = env->GetMethodID(sunMoonCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;)V");

    astro::AstroTime astroTime(astro::Jday(year, month, day), hour * 3600 + minute * 60 + second);
    jlong origTime = env->CallLongMethod(time, getTimeMethod);
    jlong currentTime = origTime;

    // Starts from previous day
    astroTime.addDay(-1);
    currentTime -= 86400000;
    acoord.setTime(astroTime);
    acoord.beginConvert();
    pl.calc(acoord);

    // Sun and moon
    astro::Vec3 sun  = pl.vecQ(astro::Planets::SUN);
    astro::Vec3 moon = pl.vecQ(astro::Planets::MOON);
    acoord.conv_q2tq(sun);
    acoord.conv_q2tq(moon);
    acoord.conv_q2h(sun);
    acoord.conv_q2h(moon);
    acoord.addRefraction(sun);
    acoord.addRefraction(moon);

    astro::AstroTime t = acoord.getTime();
    // Calculate up to 2 days
    const double jd_end = t.jd() + 2;
    const double sun_rz  = sin(astro::dms2rad(0,0,960));
    const double min30_z = sin(astro::hms2rad(0,30,0));
    const double min3_z  = sin(astro::hms2rad(0,3,0));
    const double sec15_z = sin(astro::hms2rad(0,0,15));
    int step = -1;    // Backward 1s to calculate

    t.addSec(step);
    currentTime += step * 1000;

    jobject sunpeak = NULL;
    jobject moonpeak = NULL;
    jobject sunrise = NULL;
    jobject moonrise = NULL;
    jobject sunriset = NULL;
    jobject moonriset = NULL;

    for (; t.jd() < jd_end; t.addSec(step)) {
        // Save last data for comparation
        const astro::Vec3 sun0 = sun;
        const astro::Vec3 moon0 = moon;
        // Calculate current position
        acoord.setTime(t);
        acoord.beginConvert();
        pl.calc(acoord);
        sun  = pl.vecQ(astro::Planets::SUN);
        moon = pl.vecQ(astro::Planets::MOON);
        acoord.conv_q2tq(sun);
        acoord.conv_q2tq(moon);
        acoord.conv_q2h(sun);
        acoord.conv_q2h(moon);
        acoord.addRefraction(sun);
        acoord.addRefraction(moon);
        sun.z += sun_rz;
        moon.z += sun_rz;
        if (step > 0) {
            astro::Degree az0, el0, az, el;
            double azDeg, elDeg, elDeg0;

            // Sun
            if (sunriset == NULL) {
                sun.getLtLg(el, az);
                sun0.getLtLg(el0, az0);
                // South 0, East 90, North 180, West 270
                az.mod360();
                azDeg = az.degree();
                elDeg = el.degree();
                elDeg0 = el0.degree();

                if (elDeg0 < 0 && elDeg >= 0) {
                    // Sunrise
                    if (sunrise != NULL) {
                        env->DeleteLocalRef(sunrise);
                        env->DeleteLocalRef(sunpeak);
                        sunrise = NULL;
                        sunpeak = NULL;
                    }
                    sunrise = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                    sunpeak = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                } else if (elDeg0 >=0 && elDeg < 0) {
                    // Sunset
                    if (currentTime - origTime < 0) {
                        // Already happened, disregard & clear
                        if (sunrise != NULL) {
                            env->DeleteLocalRef(sunrise);
                            env->DeleteLocalRef(sunpeak);
                            sunrise = NULL;
                            sunpeak = NULL;
                        }
                    } else if (sunrise != NULL) {
                        // Disregard if there is no rise/peak
                        jobject sunset = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                        sunriset = env->NewObject(risetCls, risetInitMethod, sunrise, sunset, sunpeak, env->NewStringUTF("Sun"));
                    }
                } else if (elDeg > elDeg0 && sunpeak != NULL && sunrise != NULL) {
                    env->CallVoidMethod(sunpeak, updatePosMehod, elDeg, azDeg, currentTime);
                }
            }

            if (moonriset == NULL) {
                // Moon
                moon.getLtLg(el, az);
                moon0.getLtLg(el0, az0);
                // South 0, East 90, North 180, West 270
                az.mod360();
                azDeg = az.degree();
                elDeg = el.degree();
                elDeg0 = el0.degree();

                if (elDeg0 < 0 && elDeg >= 0) {
                    // Moonrise
                    if (moonrise != NULL) {
                        env->DeleteLocalRef(moonrise);
                        env->DeleteLocalRef(moonpeak);
                        moonrise = NULL;
                        moonpeak = NULL;
                    }
                    moonrise = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                    moonpeak = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                } else if (elDeg0 >=0 && elDeg < 0) {
                    // Moonset
                    if (currentTime - origTime < 0) {
                        // Already happened, disregard & clear
                        if (moonrise != NULL) {
                            env->DeleteLocalRef(moonrise);
                            env->DeleteLocalRef(moonpeak);
                            moonrise = NULL;
                            moonpeak = NULL;
                        }
                    } else if (moonrise != NULL) {
                        // Disregard if there is no rise/peak
                        jobject moonset = env->NewObject(posCls, posInitMethod, elDeg, azDeg, currentTime);
                        moonriset = env->NewObject(risetCls, risetInitMethod, moonrise, moonset, moonpeak, env->NewStringUTF("Moon"));
                    }
                } else if (elDeg > elDeg0 && moonpeak != NULL && moonrise != NULL) {
                    env->CallVoidMethod(moonpeak, updatePosMehod, elDeg, azDeg, currentTime);
                }
            }
        }

        if (sunriset != NULL && moonriset != NULL) {
            break;
        }

        // Calculate the step size
        double z = min(fabs(sun.z), fabs(moon.z));
        double y = min(fabs(sun.y), fabs(moon.y));
        z = min(z, y);
        if (z >= min30_z)
            step = 20*60;
        else if (z >= min3_z)
            step = 2*60;
        else if (z >= sec15_z)
            step = 10;
        else
            step = 1;

        // Recalculate current time
        currentTime += step * 1000;
    }
    return env->NewObject(sunMoonCls, sunMoonInitMethod, sunriset, moonriset);
}


jobject getAllRiset(JNIEnv *env,
                    jdouble longitude, jdouble latitude,
                    jobject time) {
    const char *planets[] = {
        "Sun",
        "Moon",
        "Mercury",
        "Venus",
        "Mars",
        "Jupiter",
        "Saturn",
        "Uranus",
        "Neptune",
        "Pluto",
    };

    astro::Degree lt(latitude * 3600);
    astro::Degree lg(longitude * 3600);
    double sea = 0;

    astro::AstroCoordinate acoord;
    astro::Planets pl;
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, sea);

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

    jclass dateCls = env->FindClass("java/util/Date");
    jmethodID getTimeMethod = env->GetMethodID(dateCls, "getTime", "()J");

    jclass posCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroPosition");
    jclass risetCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/AstroRiset");
    jmethodID posInitMethod = env->GetMethodID(posCls, "<init>", "(DDJ)V");
    jmethodID updatePosMehod = env->GetMethodID(posCls, "update", "(DDJ)V");
    jmethodID risetInitMethod = env->GetMethodID(risetCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Lcc/meowssage/astroweather/SunMoon/Model/AstroPosition;Ljava/lang/String;)V");

    jclass sunMoonCls = env->FindClass("cc/meowssage/astroweather/SunMoon/Model/SunMoonRiset");
    jmethodID sunMoonInitMethod = env->GetMethodID(sunMoonCls, "<init>", "(Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;Lcc/meowssage/astroweather/SunMoon/Model/AstroRiset;)V");

    astro::AstroTime astroTime(astro::Jday(year, month, day), hour * 3600 + minute * 60 + second);
    jlong origTime = env->CallLongMethod(time, getTimeMethod);
    jlong currentTime = origTime;

    // Starts from previous day
    astroTime.addDay(-1);
    currentTime -= 86400000;
    acoord.setTime(astroTime);
    acoord.beginConvert();
    pl.calc(acoord);

    // record the first calculated plvs
    std::vector<astro::Vec3> plvs;
    std::vector<std::vector<std::pair<jlong, bool>>> temps;
    for (int i = astro::Planets::SUN; i <= astro::Planets::PLUTO; i++)
    {
        astro::Vec3 vec = pl.vecQ(i);
        acoord.conv_q2tq(vec);
        acoord.conv_q2h(vec);
        // 大気差補正は常時実施する.
        acoord.addRefraction(vec);
        if (i == astro::Planets::SUN || i == astro::Planets::MOON)
        {
            vec.z += sin(astro::dms2rad(0,0,960)); // 太陽視半径分を高度補正する.
        }
        plvs.push_back(pl.vecQ(i));
        std::vector<std::pair<jlong, bool>> empty;
        temps.push_back(empty);
    }

    astro::AstroTime t = acoord.getTime();
    const double jd_end = t.jd() + 2;
    const int step = 60;
    currentTime += 60000;
    for (t.addSec(step); t.jd() < jd_end; t.addSec(step)) {
        // 前回時刻の高度を保存する. ただし、初回はこの値を使ってはいけない.
        std::vector<astro::Vec3> newPlvs;
        // 今回時刻の高度を計算する.
        acoord.setTime(t);
        acoord.beginConvert();
        pl.calc(acoord);
        for (int i = astro::Planets::SUN; i <= astro::Planets::PLUTO; i++)
        {
            astro::Vec3 vec = pl.vecQ(i);
            acoord.conv_q2tq(vec);
            acoord.conv_q2h(vec);
            // 大気差補正は常時実施する.
            acoord.addRefraction(vec);
            if (i == astro::Planets::SUN || i == astro::Planets::MOON)
            {
                vec.z += sin(astro::dms2rad(0,0,960)); // 太陽視半径分を高度補正する.
            }
            if (vec.z >= 0 && plvs[i].z < 0)
            {
                temps[i].push_back(std::make_pair(currentTime, true));
            }
            if (vec.z < 0 && plvs[i].z >= 0)
            {
                temps[i].push_back(std::make_pair(currentTime, false));
            }
            newPlvs.push_back(vec);
        }
        plvs = newPlvs;

        // Recalculate current time
        currentTime += step * 1000;
    }

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID construct = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject result = env->NewObject(arrayListClass, construct, temps.size());
    jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    for (int i = 0; i < temps.size(); i++) {
        for (int j = 0; j < temps[i].size() - 1; j++) {
            jlong curr = temps[i][j].first;
            jlong next = temps[i][j + 1].first;
            if (temps[i][j].second && !temps[i][j + 1].second && next > origTime) {
                jobject rise = env->NewObject(posCls, posInitMethod, 0.0, 0.0, curr);
                jobject set = env->NewObject(posCls, posInitMethod, 0.0, 0.0, next);
                jobject riset = env->NewObject(risetCls, risetInitMethod, rise, set, NULL, env->NewStringUTF(planets[i]));
                env->CallBooleanMethod(result, arrayListAdd, riset);
                break;
            }
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
