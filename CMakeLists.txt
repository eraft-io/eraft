cmake_minimum_required (VERSION 2.6)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Linux
if (EXISTS "/opt/rh/devtoolset-8/root/usr/bin/gcc")
    set(GCC_8_PATH "/opt/rh/devtoolset-8/root/usr")
    set(CMAKE_C_COMPILER  ${GCC_8_PATH}/bin/gcc)
    set(CMAKE_CXX_COMPILER  ${GCC_8_PATH}/bin/g++)
    set(CMAKE_CXX_FLAGS "-DCOMPATIBLE_SLAVE -pipe -std=c++11 -fpermissive -fPIC -pthread -W -Wextra -Wall -Wsign-compare -Wshadow -Wno-unused-parameter -static-libgcc -static-libstdc++")
    set(CMAKE_C_FLAGS "-DCOMPATIBLE_SLAVE -pipe -std=c++11 -fpermissive -fPIC -pthread -W -Wextra -Wall -Wsign-compare -Wshadow -Wno-unused-parameter -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 -D__STDC_FORMAT_MACROS")
    set(CMAKE_CXX_FLAGS_RELEASE "-ggdb -O2 -DNDEBUG -DWITH_TCMALLOC -D__STDC_FORMAT_MACROS")
    else()
endif()

project(edb)
enable_testing()
add_subdirectory(src/parser)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BINARY_DIR build)
set(EXECUTABLE_OUTPUT_PATH build)
set(SOURCE
	${SOURCE}
	src/btree/btree.cpp
	src/fs/page_fs.cpp
	src/page/variant_page.cpp
	src/table/record.cpp
	src/table/table.cpp
	src/table/table_header.cpp
	src/database/database.cpp
	src/database/dbms.cpp
	src/expression/expression.cpp
	src/expression/serialization.cpp
	src/index/index.cpp
)

set(
	HEADERS
	${HEADERS}
)

# Google Test
set(BUILD_GTEST ON CACHE BOOL "Builds the googletest subproject" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
add_subdirectory(gtest)
set_target_properties(gtest gtest_main PROPERTIES
	FOLDER libraries
)

add_subdirectory(network)
add_subdirectory(raftcore)

add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SOURCE} ${HEADERS})
target_link_libraries(${CMAKE_PROJECT_NAME}_static PUBLIC
	network
	pthread
)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC include)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
					  sql_parser
					  pthread
					  ${CMAKE_PROJECT_NAME}_static
					  )
