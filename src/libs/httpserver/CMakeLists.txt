
if(enable_httpserver)

set(MODULE_NAME httpserver)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})

if(WIN32)
TARGET_LINK_LIBRARIES(${MODULE_NAME} libboost_random-vc143-mt-x64-1_78 libboost_program_options-vc143-mt-x64-1_78 libboost_system-vc143-mt-x64-1_78 libboost_filesystem-vc143-mt-x64-1_78 libboost_thread-vc143-mt-x64-1_78)
elseif(APPLE)
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_random boost_program_options boost_system boost_filesystem boost_thread)
else()
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_random boost_program_options boost_system boost_filesystem boost_thread)
endif()

INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

endif()
