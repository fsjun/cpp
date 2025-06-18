
if(enable_drogonws)

find_package(Drogon CONFIG REQUIRED)

set(MODULE_NAME drogonws)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})
if(WIN32)
TARGET_LINK_LIBRARIES(${MODULE_NAME} librabbitmq.4 Ws2_32)
else()
TARGET_LINK_LIBRARIES(${MODULE_NAME} Drogon::Drogon libev.a)
endif()
INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()
