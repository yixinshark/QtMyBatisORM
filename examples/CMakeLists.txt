cmake_minimum_required(VERSION 3.16)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Sql Xml)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)

# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE QtMyBatisORM Qt6::Core Qt6::Sql Qt6::Xml)

# SQLite example
add_executable(sqlite_example sqlite_example.cpp)
target_link_libraries(sqlite_example PRIVATE QtMyBatisORM Qt6::Core Qt6::Sql Qt6::Xml)

# MySQL example
add_executable(mysql_example mysql_example.cpp)
target_link_libraries(mysql_example PRIVATE QtMyBatisORM Qt6::Core Qt6::Sql Qt6::Xml)

# Copy example resources
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
    foreach(TARGET basic_example sqlite_example mysql_example)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:${TARGET}>/resources
        )
    endforeach()
endif()