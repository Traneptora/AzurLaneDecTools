cmake_minimum_required(VERSION 3.0.0)
project(bcDec VERSION 0.1.0)

set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE x86)

cmake_policy(SET CMP0074 NEW)
find_package(Boost REQUIRED COMPONENTS system filesystem)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_executable(bcDec ByteCodeDec.cpp main.cpp)
target_include_directories(bcDec PUBLIC ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src)
target_link_directories(bcDec PUBLIC ${Boost_LIBRARY_DIRS} ${PROJECT_SOURCE_DIR}/src)

IF( MSVC )
add_custom_command(TARGET bcDec PRE_BUILD
COMMAND cd /d ${PROJECT_SOURCE_DIR}/src
COMMAND ${PROJECT_SOURCE_DIR}/src/msvcbuild.bat static
COMMAND cd /d ${PROJECT_SOURCE_DIR}
)
target_link_libraries(bcDec lua51)
ENDIF ( MSVC )

