# Author:
#   Sven Czarnian <devel@svcz.de>
# License:
#   GPLv3
# Brief:
#   Creates the vACMD EuroScope plugin

CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# define the project
PROJECT(vACDM LANGUAGES C CXX VERSION "1.0.0")
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
IF (MSVC)
    IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    ELSE ()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    ENDIF ()
    IF (NOT CMAKE_CXX_FLAGS MATCHES "/MP")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    ENDIF ()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /sdl /permissive- /DNOMINMAX")
    SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /sdl /permissive- /DNOMINMAX")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFESTUAC:NO /ignore:4099")
    ADD_DEFINITIONS(/D_USRDLL)
ENDIF ()

CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/Version.h.in
    ${CMAKE_BINARY_DIR}/Version.h
)

# add the binary, include and installation directories to the search paths
INCLUDE_DIRECTORIES(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/external/include
    ${CMAKE_SOURCE_DIR}
)

ADD_DEFINITIONS(
    -D_CRT_SECURE_NO_WARNINGS
    -DSQLITE_THREADSAFE=0
    -DSQLITE_DEFAULT_FILE_FORMAT=4
    -DSQLITE_DEFAULT_SYNCHRONOUS=0
    -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=0
    -DSQLITE_WIN32_MALLOC
    -DSQLITE_THREADSAFE=0
)

SET(SOURCE_FILES
    config/FileFormat.cpp
    config/FileFormat.h
    com/Airport.cpp
    com/Airport.h
    com/PerformanceLock.h
    com/Server.cpp
    com/Server.h
    logging/Logger.cpp
    logging/Logger.h
    logging/Performance.h
    logging/sqlite3.c
    logging/sqlite3.h
    logging/sqlite3ext.h
    types/Flight.h
    types/SystemConfig.h
    main.cpp
    vACDM.cpp
    vACDM.h
    Version.h
)

ADD_LIBRARY(vACDM SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(vACDM ${CMAKE_SOURCE_DIR}/external/lib/EuroScopePlugInDLL.lib crypt32.lib ws2_32.lib Shlwapi.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/jsoncpp_d.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/libcurl-d.lib)
TARGET_LINK_LIBRARIES(vACDM debug ${CMAKE_SOURCE_DIR}/external/lib/Geographic_d.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/jsoncpp.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/libcurl.lib)
TARGET_LINK_LIBRARIES(vACDM optimized ${CMAKE_SOURCE_DIR}/external/lib/Geographic.lib)
