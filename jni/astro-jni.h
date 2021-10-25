#ifndef ASTRO_JNI_H
#define ASTRO_JNI_H

#include <jni.h>

jobject getRiset(JNIEnv *env, jdouble longitude, jdouble latitude, jdouble altitude, jobject time);
jobject getAllRiset(JNIEnv *env, jdouble longitude, jdouble latitude, jdouble altitude, jobject time);

jobject getLunarPhase(JNIEnv *env, jobject time);
jobject getStarRiset(JNIEnv *env, jdouble ra, jdouble dec, jdouble ra_pm, jdouble dec_pm, jdouble longitude, jdouble latitude, jdouble altitude, jobject time);
jdouble getLST(JNIEnv *env, jdouble longitude, jobject time);
jobject getSunAltTime(JNIEnv *env, jdouble longitude, jdouble latitude, jdouble altitude, jboolean go_down, jobject time, jdouble x);
jobject getSatelliteStatus(JNIEnv *env, jstring line0, jstring line1, jstring line2, jobject time);
jobject getSatelliteNextRiset(JNIEnv *env,
                              jstring line0, jstring line1,
                              jstring line2, jobject time,
                              jdouble longitude,
                              jdouble latitude,
                              jdouble altitude);
jobject getStarPosition(JNIEnv *env, jdouble ra, jdouble dec,
                        jdouble ra_pm, jdouble dec_pm,
                        jobject time,
                        jdouble longitude, jdouble latitude,
                        jdouble altitude);
jobject getSolarSystemObjectPosition(JNIEnv *env, jint index, jobject time, jdouble longitude, jdouble latitude, jdouble altitude);

#endif
