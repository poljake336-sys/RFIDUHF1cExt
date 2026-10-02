cmake_minimum_required(VERSION 3.22.1)
project(org_rfiduhf_addin)

add_library(
    org_rfiduhf_addin
    SHARED
    native-lib.cpp
    ../../../../../Inf18/AddInNative.cpp
    ../../../../../jni/jnienv.cpp
    ../../../../../Inf18/ConversionWchar.cpp
    ../../../../../Inf18/MainApp.cpp
)

include_directories(../../../../../include)

find_library(log-lib log)

target_link_libraries(
    org_rfiduhf_addin
    ${log-lib}
)
