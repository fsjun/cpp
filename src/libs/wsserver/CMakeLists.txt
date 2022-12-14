
if(enable_wsserver)

set(MODULE_NAME wsserver)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})

if(WIN32)
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_random-vc140-mt-gd boost_program_options-vc140-mt-gd boost_system-vc140-mt-gd boost_filesystem-vc140-mt-gd boost_thread-vc140-mt-gd)
elseif(APPLE)
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_random boost_program_options boost_system boost_filesystem boost_thread)
else()
TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_random boost_program_options boost_system boost_filesystem boost_thread)
endif()

INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

endif()
