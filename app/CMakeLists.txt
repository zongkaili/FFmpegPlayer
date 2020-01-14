cmake_minimum_required(VERSION 3.4.1)

file(GLOB SOURCE src/main/cpp/*.cpp)
add_library( # Sets the name of the library.
        native-lib
        SHARED
        ${SOURCE})

include_directories(src/main/cpp/include)

set(my_lib_path ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${my_lib_path}")

find_library( # Sets the name of the path variable.
        log-lib
        log)

target_link_libraries( # Specifies the target library.
        native-lib
        avfilter avformat avcodec avutil swresample swscale
        ${log-lib}
        android
        z
        OpenSLES)