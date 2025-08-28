IF(BUILD_TESTING)
    FIND_PACKAGE(GTest REQUIRED)

    # Unit tests
    ADD_EXECUTABLE(unit_tests
        backend/test_JsonBuilder.cpp
        # log/test_ConsoleLogger.cpp
        # log/test_SqlLiteLogger.cpp
        utils/test_StringUtils.cpp
        test_main.cpp
    )
    TARGET_LINK_LIBRARIES(unit_tests PRIVATE
        ${PROJECT_NAME}_static
        GTest::gtest_main
        GTest::gtest
    )
    TARGET_INCLUDE_DIRECTORIES(unit_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/external/include
    )
    INCLUDE(GoogleTest)
    gtest_discover_tests(unit_tests
        PROPERTIES
        LABELS "unit"
    )

    # Integration tests
    ADD_EXECUTABLE(integration_tests
        integration/api/test_CurlRestClient.cpp
        integration/backend/test_Nats.cpp
        integration/backend/test_Backend.cpp
        test_main.cpp
    )
    TARGET_LINK_LIBRARIES(integration_tests PRIVATE
        ${PROJECT_NAME}_static
        GTest::gtest_main
        GTest::gtest
    )
    TARGET_INCLUDE_DIRECTORIES(integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/external/include
    )
    gtest_discover_tests(integration_tests
        PROPERTIES
        LABELS "integration"
    )
ENDIF()
