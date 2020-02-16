
file(GLOB rtmp_srcs *.c)
#-D 定义宏   NO_CRYPTO
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library(
        rtmp
        STATIC
        ${rtmp_srcs}
)