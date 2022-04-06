# Author:
#   Sven Czarnian <devel@svcz.de>
# Copyright:
#   2021 Sven Czarnian
# License:
#   GNU General Public License (GPLv3)
# Brief:
#   Defines the different externals

INCLUDE(ExternalProject)

ExternalProject_Add(
    eigen_build
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.3.7
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DEIGEN_TEST_CXX11=OFF
        -DEIGEN_BUILD_BTL=OFF
        -DBUILD_TESTING=OFF
)

ADD_LIBRARY(eigen INTERFACE)
TARGET_INCLUDE_DIRECTORIES(eigen INTERFACE "${CMAKE_INSTALL_PREFIX}/include/eigen3")
ADD_DEPENDENCIES(eigen eigen_build)

ExternalProject_Add(
    gsl_build
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG v3.1.0
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DGSL_TEST=OFF
)

ADD_LIBRARY(gsl INTERFACE)
TARGET_INCLUDE_DIRECTORIES(gsl INTERFACE "${CMAKE_INSTALL_PREFIX}/include")
ADD_DEPENDENCIES(gsl gsl_build)

ExternalProject_Add(
    geographiclib_build
    GIT_REPOSITORY https://git.code.sf.net/p/geographiclib/code
    GIT_TAG release
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCPACK_BINARY_NSIS=OFF
        -DGEOGRAPHICLIB_DATA=${CMAKE_INSTALL_PREFIX}
        -DGEOGRAPHICLIB_PRECISION=1
)

ADD_LIBRARY(geographiclib STATIC IMPORTED)
ADD_DEPENDENCIES(geographiclib geographiclib_build)
IF (MSVC)
    SET_TARGET_PROPERTIES(geographiclib PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_INSTALL_PREFIX}/lib/Geographic_d.lib"
        IMPORTED_LOCATION_RELEASE "${CMAKE_INSTALL_PREFIX}/lib/Geographic.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_INSTALL_PREFIX}/lib/Geographic.lib"
    )
    TARGET_INCLUDE_DIRECTORIES(geographiclib INTERFACE "${CMAKE_INSTALL_PREFIX}/include")
ELSE ()
    MESSAGE(FATAL_ERROR "Unsupported compiler")
ENDIF ()

ExternalProject_Add(
    nanoflann_build
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG v1.3.2
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DBUILD_EXAMPLES=OFF
        -DBUILD_BENCHMARKS=OFF
)

ADD_LIBRARY(nanoflann INTERFACE)
TARGET_INCLUDE_DIRECTORIES(nanoflann INTERFACE "${CMAKE_INSTALL_PREFIX}/include")
ADD_DEPENDENCIES(nanoflann nanoflann_build)

ExternalProject_Add(
    boost_build
    URL https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.zip
    URL_HASH SHA256=d2886ceff60c35fc6dc9120e8faa960c1e9535f2d7ce447469eae9836110ea77
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    SOURCE_DIR ${PROJECT_BINARY_DIR}/boost
)

ADD_LIBRARY(boost INTERFACE)
TARGET_INCLUDE_DIRECTORIES(boost INTERFACE "${PROJECT_BINARY_DIR}/boost")
ADD_DEPENDENCIES(boost boost_build)

ExternalProject_Add(
    curl_build
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-7_71_1
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DPICKY_COMPILER=OFF
        -DBUILD_CURL_EXE=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DENABLE_ARES=OFF
        -DENABLE_DEBUG=OFF
        -DENABLE_CURLDEBUG=OFF
        -DHTTP_ONLY=ON
        -DCURL_DISABLE_COOKIES=ON
        -DCURL_DISABLE_CRYPTO_AUTH=ON
        -DCURL_DISABLE_VERBOSE_STRINGS=ON
        -DCMAKE_USE_WINSSL=ON
)

ADD_LIBRARY(curl STATIC IMPORTED)
ADD_DEPENDENCIES(curl curl_build)
IF (MSVC)
    SET_TARGET_PROPERTIES(curl PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_INSTALL_PREFIX}/lib/libcurl-d.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_INSTALL_PREFIX}/lib/libcurl.lib"
    )
    TARGET_INCLUDE_DIRECTORIES(curl INTERFACE "${CMAKE_INSTALL_PREFIX}/include")
ELSE ()
    MESSAGE(FATAL_ERROR "Unsupported compiler")
ENDIF ()

SET_TARGET_PROPERTIES(eigen_build         PROPERTIES FOLDER "external")
SET_TARGET_PROPERTIES(gsl_build           PROPERTIES FOLDER "external")
SET_TARGET_PROPERTIES(geographiclib_build PROPERTIES FOLDER "external")
SET_TARGET_PROPERTIES(nanoflann_build     PROPERTIES FOLDER "external")
SET_TARGET_PROPERTIES(boost_build         PROPERTIES FOLDER "external")
SET_TARGET_PROPERTIES(curl_build          PROPERTIES FOLDER "external")
