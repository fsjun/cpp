
MACRO(SUBDIRLIST result curdir)
FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
SET(dirlist "")
FOREACH(child ${children})
  IF(IS_DIRECTORY ${curdir}/${child})
    LIST(APPEND dirlist ${child})
  ENDIF()
ENDFOREACH()
SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(SUBDIRS ${CMAKE_SOURCE_DIR}/src/libs)

FOREACH(subdir ${SUBDIRS})
  ADD_SUBDIRECTORY(${subdir})
ENDFOREACH()
