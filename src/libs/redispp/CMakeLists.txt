
if(enable_redispp)
set(ENV{PKG_CONFIG_PATH} "/opt/app/3rd/redispp/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "/opt/app/vcpkg/installed/x64-linux/debug/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)
message("ENV{PKG_CONFIG_PATH} is $ENV{PKG_CONFIG_PATH}")

pkg_search_module(REDISPP REQUIRED redis++)
include_directories(${REDISPP_INCLUDE_DIRS})
LINK_DIRECTORIES(${REDISPP_LIBRARY_DIRS})
message("${REDISPP_INCLUDE_DIRS} ${REDISPP_LIBRARY_DIRS} ${REDISPP_LIBRARIES}")

set(MODULE_NAME redispp)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})
if(WIN32)
TARGET_LINK_LIBRARIES(${MODULE_NAME} crypt32 Ws2_32 Wldap32 Normaliz)
else()
TARGET_LINK_LIBRARIES(${MODULE_NAME} libhiredisd.a libredis++.a)
endif()
INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()
