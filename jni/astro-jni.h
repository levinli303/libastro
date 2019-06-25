#ifndef ASTRO_JNI_H
#define ASTRO_JNI_H

#include <jni.h>

jobject getRiset(JNIEnv *env, jdouble longitude, jdouble latitude, jdouble altitude, jobject time);
jobject getAllRiset(JNIEnv *env, jdouble longitude, jdouble latitude, jdouble altitude, jobject time);

jobject getLunarPhase(JNIEnv *env, jobject time);

#endif
