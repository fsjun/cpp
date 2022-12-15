cmake_minimum_required (VERSION 3.11)
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(WIN32)
message("WIN32 platform")
message("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
elseif(APPLE)
message("APPLE platform")
elseif(LINUX)
message("LINUX platform")
#set(CMAKE_C_COMPILER "/usr/local/gcc/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/local/gcc/bin/g++")
elseif(UNIX)
message("UNIX platform")
else()
message("unknown platform")
endif()

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project (cpp)

if(WIN32)

SET(CMAKE_INSTALL_PREFIX "D:/program/cpp")

#ADD_DEFINITIONS(-DCMAKE_BUILD_TYPE=DEBUG)
#include_directories(D:/program/vcpkg/installed/x64-windows/include)
#LINK_DIRECTORIES(D:/program/vcpkg/installed/x64-windows/debug/lib)
#LINK_DIRECTORIES(D:/program/vcpkg/installed/x64-windows/debug/lib/manual-link)
include_directories(D:/program/vcpkg/installed/x64-windows-static/include)
LINK_DIRECTORIES(D:/program/vcpkg/installed/x64-windows-static/debug/lib)
LINK_DIRECTORIES(D:/program/vcpkg/installed/x64-windows-static/debug/lib/manual-link)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4430 /utf-8")

if (MSVC)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif(MSVC)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
add_definitions(-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601)

elseif(APPLE)

SET(CMAKE_INSTALL_PREFIX "/usr/local/3rd/cpp")

include_directories(/root/app/vcpkg/installed/x64-linux/include)
LINK_DIRECTORIES(/root/app/vcpkg/installed/x64-linux/debug/lib)
LINK_DIRECTORIES(/root/app/vcpkg/installed/x64-linux/debug/lib/manual-link)
include_directories("/usr/local/include")
LINK_DIRECTORIES(/usr/local/lib)

elseif(LINUX)

SET(CMAKE_INSTALL_PREFIX "/usr/local/3rd/cpp")

link_directories(/usr/lib64)

include_directories(/root/app/vcpkg/installed/x64-linux/include)
LINK_DIRECTORIES(/root/app/vcpkg/installed/x64-linux/debug/lib)
LINK_DIRECTORIES(/root/app/vcpkg/installed/x64-linux/debug/lib/manual-link)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O0 -ggdb")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
else()
endif()

ADD_DEFINITIONS(-DBOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX -DBOOST_ASIO_DISABLE_SMALL_BLOCK_RECYCLING)

if(WIN32)
else()
set(enable_database TRUE)
set(enable_redis TRUE)
endif()

set(enable_jsoncc TRUE)
set(enable_ossl TRUE)
set(enable_base64 TRUE)
set(enable_httpclient TRUE)
set(enable_httpserver TRUE)
set(enable_wsclient TRUE)
set(enable_wsserver TRUE)
#set(enable_sipserver TRUE)

add_subdirectory(src)
