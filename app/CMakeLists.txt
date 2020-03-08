cmake_minimum_required(VERSION 3.4.1)

file(GLOB SOURCE src/main/cpp/*.cpp)

#引入指定目录下的CMakeLists.txt
add_subdirectory(src/main/cpp/librtmp)

include_directories(src/main/cpp/librtmp)

include_directories(src/main/cpp/x264/include)

include_directories(src/main/cpp/libfaac/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/cpp/x264/libs/${CMAKE_ANDROID_ARCH_ABI}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/cpp/libfaac/libs/${CMAKE_ANDROID_ARCH_ABI}")

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
        OpenSLES
        rtmp
        x264
        faac)