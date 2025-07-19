cmake_minimum_required(VERSION 3.16)

# IDE compatibility settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Project definition
project(QtMyBatisORM 
    VERSION 1.0.0 
    DESCRIPTION "Qt-based MyBatis-like ORM library"
    HOMEPAGE_URL "https://github.com/yourusername/QtMyBatisORM"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# IDE-friendly configurations
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Platform-specific settings and compiler optimizations
if(WIN32)
    # Windows-specific settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    if(MSVC)
        # MSVC-specific settings
        add_compile_options(/W4 /MP)
        # Release optimizations for MSVC
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_compile_options(/Ox /Ob2 /Ot)
        endif()
    endif()
elseif(APPLE)
    # macOS-specific settings
    set(CMAKE_MACOSX_RPATH ON)
    add_compile_options(-Wall -Wextra)
    # Release optimizations for Apple Clang
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    add_compile_options(-Wall -Wextra)
    # Release optimizations for GCC/Clang
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O2 -DNDEBUG -march=native)
    endif()
    # Debug information
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
endif()

# Find Qt6 components with detailed configuration and error handling
find_package(Qt6 6.0 REQUIRED COMPONENTS Core Sql Xml)

# Verify Qt6 installation
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please ensure Qt6 is installed and CMAKE_PREFIX_PATH is set correctly.")
endif()

# Find Qt6 components with detailed configuration
find_package(Qt6 REQUIRED COMPONENTS Core Sql Xml)

# Qt configuration for IDE compatibility  
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt specific settings
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
set(QT_QMAKE_EXECUTABLE_FINDQT TRUE)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# IDE-friendly include paths
set(QtMyBatisORM_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QtMyBatisORM
    CACHE PATH "QtMyBatisORM include directories"
)

# Source files
set(SOURCES
    src/core/configurationmanager.cpp
    src/core/sessionfactory.cpp
    src/core/session.cpp
    src/core/executor.cpp
    src/core/statementhandler.cpp
    src/core/parameterhandler.cpp
    src/core/resulthandler.cpp
    src/core/dynamicsqlprocessor.cpp
    src/core/logger.cpp
    src/pool/connectionpool.cpp
    src/pool/connectionpool_monitor.cpp
    src/cache/cachemanager.cpp
    src/mapper/mapperregistry.cpp
    src/mapper/mapperproxy.cpp
    src/config/jsonconfigparser.cpp
    src/config/xmlmapperparser.cpp
    src/exception/qtmybatisexception.cpp
    src/qtmybatisorm.cpp
)

# Header files
set(HEADERS
    include/QtMyBatisORM/configurationmanager.h
    include/QtMyBatisORM/sessionfactory.h
    include/QtMyBatisORM/sessionfactory_impl.h
    include/QtMyBatisORM/session.h
    include/QtMyBatisORM/session_impl.h
    include/QtMyBatisORM/executor.h
    include/QtMyBatisORM/statementhandler.h
    include/QtMyBatisORM/parameterhandler.h
    include/QtMyBatisORM/resulthandler.h
    include/QtMyBatisORM/dynamicsqlprocessor.h
    include/QtMyBatisORM/logger.h
    include/QtMyBatisORM/connectionpool.h
    include/QtMyBatisORM/cachemanager.h
    include/QtMyBatisORM/mapperregistry.h
    include/QtMyBatisORM/mapperproxy.h
    include/QtMyBatisORM/jsonconfigparser.h
    include/QtMyBatisORM/xmlmapperparser.h
    include/QtMyBatisORM/qtmybatisexception.h
    include/QtMyBatisORM/objectpool.h
    include/QtMyBatisORM/DataModels.h
    include/QtMyBatisORM/qtmybatisorm.h
)

# Create library
add_library(QtMyBatisORM ${SOURCES} ${HEADERS})

# Set target properties with IDE support
set_target_properties(QtMyBatisORM PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    EXPORT_NAME QtMyBatisORM
    OUTPUT_NAME QtMyBatisORM
    DEBUG_POSTFIX d
    
    # IDE settings
    FOLDER "Libraries"
    
    # Development aids
    POSITION_INDEPENDENT_CODE ON
)

# Define target compile definitions
target_compile_definitions(QtMyBatisORM
    PRIVATE
        QT_USE_QSTRINGBUILDER
        QT_STRICT_ITERATORS
        QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
        QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Define target include directories
target_include_directories(QtMyBatisORM
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Qt6 libraries
target_link_libraries(QtMyBatisORM
    PUBLIC
        Qt6::Core
        Qt6::Sql
        Qt6::Xml
)

# Generate and install CMake config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QtMyBatisORMConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtMyBatisORMConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QtMyBatisORMConfig.cmake"
    INSTALL_DESTINATION lib/cmake/QtMyBatisORM
)

# Install targets
install(TARGETS QtMyBatisORM
    EXPORT QtMyBatisORMTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT QtMyBatisORMTargets
    FILE QtMyBatisORMTargets.cmake
    NAMESPACE QtMyBatisORM::
    DESTINATION lib/cmake/QtMyBatisORM
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QtMyBatisORMConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QtMyBatisORMConfigVersion.cmake"
    DESTINATION lib/cmake/QtMyBatisORM
)

install(DIRECTORY include/QtMyBatisORM DESTINATION include)

# Build examples if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build documentation if enabled
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, documentation will not be built")
    endif()
endif()

# Enable testing if enabled
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()