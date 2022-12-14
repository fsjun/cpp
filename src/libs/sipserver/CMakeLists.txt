
if(enable_sipserver)

set(MODULE_NAME sipserver)
file(GLOB_RECURSE SRC_LIST *.h *.cpp)
#add_library(${MODULE_NAME} SHARED ${SRC_LIST})
add_library(${MODULE_NAME} ${SRC_LIST})
if(WIN32)
    TARGET_LINK_LIBRARIES(${MODULE_NAME} boost_program_options-vc140-mt-gd boost_system-vc140-mt-gd crypt32 Ws2_32 Wldap32 Normaliz)
elseif(APPLE)
    TARGET_LINK_LIBRARIES(${MODULE_NAME} pjsip-ua-x86_64-apple-darwin20.5.0 pjsip-x86_64-apple-darwin20.5.0 pjmedia-x86_64-apple-darwin20.5.0 pjlib-util-x86_64-apple-darwin20.5.0 pj-x86_64-apple-darwin20.5.0)
else()
    TARGET_LINK_LIBRARIES(${MODULE_NAME} pjsip-ua-x86_64-unknown-linux-gnu pjsip-x86_64-unknown-linux-gnu pjmedia-x86_64-unknown-linux-gnu pjlib-util-x86_64-unknown-linux-gnu pj-x86_64-unknown-linux-gnu)
endif()
INSTALL(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

endif()
