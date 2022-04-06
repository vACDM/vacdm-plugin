# Author:
#   Sven Czarnian <devel@svcz.de>
# License:
#   LGPLv3
# Brief:
#   Finds the EuroScope headers and libraries
#   A target EuroScope will be created and the EuroScope_FOUND flag will be set

IF(NOT TARGET EuroScope)
    IF(NOT EuroScope_DIR)
        MESSAGE(FATAL_ERROR "Please set EuroScope_DIR")
        SET(EuroScope_DIR "EuroScope_DIR-NOTFOUND" CACHE PATH PARENT_SCOPE)
    ENDIF()

    FIND_FILE(EuroScope_EXECUTABLE
        NAMES
            EuroScope.exe
        PATHS
            ${EuroScope_DIR}
    )
    FIND_FILE(EuroScope_LIBRARY
        NAMES
            EuroScopePlugInDll.lib
        PATHS
            ${EuroScope_DIR}/PlugInEnvironment
    )
    FIND_PATH(EuroScope_INCLUDE_DIR
        NAMES
            EuroScopePlugIn.h
        PATHS
            ${EuroScope_DIR}/PlugInEnvironment
    )

    IF(NOT ${EuroScope_EXECUTABLE} STREQUAL "EuroScope_EXECUTABLE-NOTFOUND" AND
       NOT ${EuroScope_LIBRARY} STREQUAL "EuroScope_LIBRARY-NOTFOUND" AND
       NOT ${EuroScope_INCLUDE_DIR} STREQUAL "EuroScope_INCLUDE_DIR-NOTFOUND")
        MESSAGE(STATUS "Found EuroScope-library:")
        MESSAGE(STATUS "  ${EuroScope_LIBRARY}")
        MESSAGE(STATUS "Found EuroScope-headers:")
        MESSAGE(STATUS "  ${EuroScope_INCLUDE_DIR}")

        ADD_LIBRARY(EuroScope INTERFACE IMPORTED GLOBAL)
        TARGET_LINK_LIBRARIES(EuroScope INTERFACE ${EuroScope_LIBRARY})
        TARGET_INCLUDE_DIRECTORIES(EuroScope INTERFACE ${EuroScope_INCLUDE_DIR})
        SET(EuroScope_FOUND ON)
    ENDIF()
ELSE()
    MESSAGE(STATUS "EuroScope is already included.")
ENDIF()
