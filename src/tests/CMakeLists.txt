IF(BUILD_TESTING)
    FIND_PACKAGE(GTest REQUIRED)

    ADD_EXECUTABLE(tests
        backend/test_Nats.cpp
        backend/test_Backend.cpp
        backend/test_JsonBuilder.cpp
        log/test_ConsoleLogger.cpp
        utils/test_StringUtils.cpp
        test_main.cpp
    )
    
    TARGET_LINK_LIBRARIES(tests PRIVATE
        ${PROJECT_NAME}_static 
        GTest::gtest_main
        GTest::gtest
    )

    TARGET_INCLUDE_DIRECTORIES(tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src 
        ${CMAKE_BINARY_DIR}      
        ${CMAKE_SOURCE_DIR}/external/include  
    )
    
    INCLUDE(GoogleTest)
    gtest_discover_tests(tests)
ENDIF()