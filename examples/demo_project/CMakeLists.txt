cmake_minimum_required(VERSION 3.16)
project(QtMyBatisORM_Demo)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# 查找QtMyBatisORM
find_package(QtMyBatisORM REQUIRED)

# 启用Qt的自动功能
qt6_standard_project_setup()

# 收集源文件
set(SOURCES
    main.cpp
    models/student.cpp
    models/course.cpp
    dao/studentdao.cpp
    dao/coursedao.cpp
    service/educationservice.cpp
)

set(HEADERS
    models/student.h
    models/course.h
    dao/studentdao.h
    dao/coursedao.h
    service/educationservice.h
)

# 创建可执行文件
qt6_add_executable(qtmybatis_demo 
    ${SOURCES}
    ${HEADERS}
)

# 添加资源文件
qt6_add_resources(qtmybatis_demo "demo_resources"
    PREFIX "/"
    OPTIONS --no-compress
    FILES
        resources/config/database.json
        resources/sql/init.sql
        resources/sql/student.sql
        resources/sql/course.sql
        resources/sql/enrollment.sql
)

# 设置include路径指向主项目
target_include_directories(qtmybatis_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# 直接链接到本地构建的库文件
target_link_libraries(qtmybatis_demo PRIVATE 
    Qt6::Core 
    Qt6::Sql
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libQtMyBatisORMd.so
)

# 设置RPATH以便运行时能找到库
set_target_properties(qtmybatis_demo PROPERTIES
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 设置目标属性
set_target_properties(qtmybatis_demo PROPERTIES
    OUTPUT_NAME "qtmybatis_demo"
)

# 如果在Windows上，设置控制台应用程序
if(WIN32)
    set_target_properties(qtmybatis_demo PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif() 