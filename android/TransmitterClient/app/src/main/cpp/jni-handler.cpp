//
// Created by Gursimran Singh on 10/06/18.
//


#include <jni.h>
#include <cstring>
#include <cstdio>
#include <android/log.h>
#include "headers.h"


const char *android_log_tag = "jni-handler.cpp";

extern "C"
JNIEXPORT void JNICALL
Java_com_gursimransinghhanspal_domf_1transmitterclient_activity_Main_setInformationElementsInProbeRequests(
        JNIEnv *env,
        jobject instance,
        jstring interface_name_jstring,
        jint interface_index_jint,
        jint ie_count_jint,
        jobjectArray ie_data_jobjectArray
) {
    size_t buffer_sz = 256;
    char buffer[buffer_sz];

    // convert `interface name` to char*
    const char *interface_name = env->GetStringUTFChars(interface_name_jstring,
                                                        reinterpret_cast<jboolean *>(JNI_FALSE));
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "interface-name: %s", interface_name);
    __android_log_write(ANDROID_LOG_INFO, android_log_tag, buffer);

    // convert `interface index` to int
    int interface_index = (int) interface_index_jint;
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "interface-index: %d", interface_index);
    __android_log_write(ANDROID_LOG_INFO, android_log_tag, buffer);

    // convert `ie count` to int
    int ie_count = (int) ie_count_jint;
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "ie-count: %d", ie_count);
    __android_log_write(ANDROID_LOG_INFO, android_log_tag, buffer);

    // convert `ie data` to <char **>
    char *ie_data[ie_count];
    for (int i = 0; i < ie_count; i++) {
        jstring elem = (jstring) (env->GetObjectArrayElement(ie_data_jobjectArray, i));

        ie_data[i] = new char[env->GetStringLength(elem) + 1];
        strcpy(ie_data[i], const_cast<char *>(env->GetStringUTFChars(elem,
                                                                     reinterpret_cast<jboolean *>
                                                                     (JNI_FALSE))));

        // print the ie_messages in logcat
        bzero(buffer, buffer_sz);
        snprintf(buffer, buffer_sz, "ie-data: %d, %s", i, ie_data[i]);
        __android_log_write(ANDROID_LOG_INFO, android_log_tag, buffer);
    }

    driver(const_cast<char *>(interface_name), ie_data[0], 2);

    // release memory
    env->ReleaseStringUTFChars(interface_name_jstring, interface_name);
    // -
    for (int i = 0; i < ie_count; i++) {
        jstring elem_jstring = (jstring) (env->GetObjectArrayElement(ie_data_jobjectArray, i));
        env->ReleaseStringUTFChars(elem_jstring, ie_data[i]);
    }
}