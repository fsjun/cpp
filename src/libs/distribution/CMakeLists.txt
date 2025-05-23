
if(enable_distribution)
set(MODULE_NAME distribution)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})
INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()
