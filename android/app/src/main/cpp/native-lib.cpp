#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_org_rfiduhf_addin_MainActivity_hello(JNIEnv *env, jobject /*thiz*/) {
    std::string hello = "RFIDUHF AddIn";
    return env->NewStringUTF(hello.c_str());
}
