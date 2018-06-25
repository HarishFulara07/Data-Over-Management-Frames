#include <jni.h>
#include <string>

#include "probe_stuffing.h"

extern "C" JNIEXPORT jstring
JNICALL
Java_com_gursimransinghhanspal_transmitterclient_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT void JNICALL
Java_com_gursimransinghhanspal_transmitterclient_MainActivity_setInformationElementsInProbeRequests(
        JNIEnv *env, jobject instance, jstring _interfaceName, jstring _ieData, jint _effort
) {

    size_t buffer_sz = 256;
    char buffer[buffer_sz];

    // convert `interface name` to char*
    const char *interface_name = env->GetStringUTFChars(_interfaceName,
                                                        reinterpret_cast<jboolean *>(JNI_FALSE));
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "interface-name: %s", interface_name);
    __android_log_write(ANDROID_LOG_INFO, "jni-handler.c", buffer);

    // convert `ie data` to char*
    const char *ie_data = env->GetStringUTFChars(_ieData,
                                                 reinterpret_cast<jboolean *>(JNI_FALSE));
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "ie-data: %s", ie_data);
    __android_log_write(ANDROID_LOG_INFO, "jni-handler.c", buffer);

    // convert `ie count` to int
    int effort = (int) _effort;
    bzero(buffer, buffer_sz);
    snprintf(buffer, buffer_sz, "effort: %d", effort);
    __android_log_write(ANDROID_LOG_INFO, "jni-handler.c", buffer);

    // driver(const_cast<char *>(interface_name), ie_data[0], 2);

    // release memory
    env->ReleaseStringUTFChars(_interfaceName, interface_name);
    env->ReleaseStringUTFChars(_ieData, ie_data);
}