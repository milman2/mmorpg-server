# 네트워크 모듈 라이브러리

add_library(mmorpg_network STATIC
    websocket_handler.cpp
    load_balancer.cpp
)

target_include_directories(mmorpg_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/network
)

target_link_libraries(mmorpg_network
    PRIVATE
    mmorpg_common
    Boost::system
    Boost::thread
    Boost::beast
)

target_compile_definitions(mmorpg_network PRIVATE
    MMORPG_NETWORK_EXPORTS
)


