# 테스트 CMakeLists.txt

# Google Test 찾기
find_package(GTest REQUIRED)

# 단위 테스트
add_executable(test_connection_manager
    unit/test_connection_manager.cpp
)

target_link_libraries(test_connection_manager
    PRIVATE
    mmorpg_connection_manager
    mmorpg_common
    mmorpg_network
    GTest::gtest
    GTest::gtest_main
    Boost::system
    Boost::thread
    Boost::beast
)

# 테스트 실행
enable_testing()
add_test(NAME ConnectionManagerTest COMMAND test_connection_manager)


