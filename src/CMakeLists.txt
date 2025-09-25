
#LINK_DIRECTORIES(/opt/pbx/libuv/lib /usr/lib64/mysql)
#ADD_DEFINITIONS(-DDEBUG_MEMORY)

include_directories(cppcommon)
include_directories(libs)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src/log/ DESTINATION include/log FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src/cppcommon/ DESTINATION include FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src/libs/ DESTINATION include FILES_MATCHING PATTERN "*.h")

#INSTALL(TARGETS screenrecord RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

set(MODULE_NAME unittest)
file(GLOB_RECURSE SRC_LIST test/*.h test/*.cpp)
ADD_EXECUTABLE(${MODULE_NAME} ${SRC_LIST})
if(WIN32)
    TARGET_LINK_LIBRARIES(${MODULE_NAME} gtest gtest_main httpclient cppcommon)
elseif(APPLE)
    TARGET_LINK_LIBRARIES(${MODULE_NAME} gtest gtest_main httpclient cppcommon)
else()
    TARGET_LINK_LIBRARIES(${MODULE_NAME} gtest gtest_main httpclient cppcommon)
endif()

set(MODULE_NAME cppcommon)
file(GLOB_RECURSE SRC_LIST log/*.h log/*.cpp cppcommon/*.h cppcommon/*.cpp)
add_library(${MODULE_NAME} ${SRC_LIST})
if(WIN32)
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_system-vc140-mt-gd boost_program_options-vc140-mt-gd crypt32 Ws2_32 Wldap32 Normaliz libssl libcrypto)
elseif(APPLE)
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_system boost_program_options ssl crypto)
else()
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_system boost_program_options ssl crypto pthread dl)
endif()
INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

add_subdirectory(libs)
