cmake_minimum_required(VERSION 3.20)
project(MMORPGServer VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 컴파일러 최적화 설정
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# 디버그 모드 설정
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined")
endif()

# 프로파일링 설정
option(ENABLE_PROFILING "Enable profiling support" OFF)
if(ENABLE_PROFILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

# SIMD 최적화
option(ENABLE_SIMD "Enable SIMD optimizations" ON)
if(ENABLE_SIMD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma")
endif()

# Include directories
include_directories(include)
include_directories(third_party)

# 서브디렉토리 추가
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)

# Conan 패키지 관리자 설정
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup()
endif()

# 설치 설정
install(TARGETS mmorpg_server
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# CPack 설정
set(CPACK_PACKAGE_NAME "MMORPGServer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C++ MMORPG Server")
set(CPACK_PACKAGE_VENDOR "MMORPG Team")
include(CPack)
