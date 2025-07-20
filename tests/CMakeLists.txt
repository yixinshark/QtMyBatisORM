# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Test)

# Enable Qt MOC for tests
set(CMAKE_AUTOMOC ON)

# Define common test dependencies
set(TEST_DEPENDENCIES
    QtMyBatisORM
    Qt6::Test
    Qt6::Core
    Qt6::Sql
    Qt6::Xml
)

# Create a truly minimal test that only tests data models and basic compilation
set(SIMPLE_TEST_SOURCES
    test_main.cpp
    test_datamodels.cpp
    test_configurationmanager.cpp
    test_jsonconfigparser.cpp
    test_xmlmapperparser.cpp
    test_connectionpool.cpp
    test_cachemanager.cpp
    test_logger.cpp
    test_qtmybatisexception.cpp
)

add_executable(QtMyBatisORMTests ${SIMPLE_TEST_SOURCES})

# Create separate test executables for new functionality
add_executable(QtMyBatisHelperTest test_qtmybatishelper.cpp)
target_link_libraries(QtMyBatisHelperTest ${TEST_DEPENDENCIES})

add_executable(NewConfigFormatTest test_new_config_format.cpp)
target_link_libraries(NewConfigFormatTest ${TEST_DEPENDENCIES})

add_executable(SimpleHelperTest test_simple_helper.cpp)
target_link_libraries(SimpleHelperTest ${TEST_DEPENDENCIES})

target_include_directories(QtMyBatisORMTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(QtMyBatisORMTests PRIVATE ${TEST_DEPENDENCIES})

# Try to add some individual tests that might work
# Test for basic JSON config parsing
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/run_jsonconfigparser_test.cpp)
    add_executable(JsonConfigParserTest run_jsonconfigparser_test.cpp)
    target_include_directories(JsonConfigParserTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
    target_link_libraries(JsonConfigParserTest PRIVATE ${TEST_DEPENDENCIES})
endif()

# Test for XML mapper parsing
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/run_xmlmapperparser_test.cpp)
    add_executable(XMLMapperParserTest run_xmlmapperparser_test.cpp)
    target_include_directories(XMLMapperParserTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
    target_link_libraries(XMLMapperParserTest PRIVATE ${TEST_DEPENDENCIES})
endif()

# Add more individual tests that are likely to work
macro(add_individual_test test_name)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/run_${test_name}_test.cpp)
        add_executable(${test_name}Test run_${test_name}_test.cpp)
        target_include_directories(${test_name}Test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )
        target_link_libraries(${test_name}Test PRIVATE ${TEST_DEPENDENCIES})
    endif()
endmacro()

# Add individual tests for core components
add_individual_test(configurationmanager)
add_individual_test(connectionpool)
add_individual_test(connectionpool_monitoring)
add_individual_test(cachemanager)
add_individual_test(executor)
add_individual_test(parameterhandler)
add_individual_test(resulthandler)
add_individual_test(dynamicsqlprocessor)
add_individual_test(session)
add_individual_test(sessionfactory)
add_individual_test(mapperregistry)
add_individual_test(mapperproxy)
add_individual_test(logger)
add_individual_test(qtmybatisexception)

# Add integration tests - 逐步启用
add_individual_test(exception_integration)
add_individual_test(session_mapper_integration)
add_individual_test(session_transaction)
add_individual_test(qtmybatisorm)
add_individual_test(performance_benchmark)
add_individual_test(cache_integration)
add_individual_test(end_to_end_integration)
add_individual_test(multi_database_integration)

# Enable testing
enable_testing()

add_test(NAME SmokeTest COMMAND QtMyBatisORMTests)

# Copy test resources
add_custom_command(TARGET QtMyBatisORMTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
)