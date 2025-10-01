# Connection Manager Agent 라이브러리

add_library(mmorpg_connection_manager STATIC
    connection_manager.cpp
)

target_include_directories(mmorpg_connection_manager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/agents/connection_manager
)

target_link_libraries(mmorpg_connection_manager
    PRIVATE
    mmorpg_common
    mmorpg_network
    Boost::system
    Boost::thread
    Boost::beast
)

target_compile_definitions(mmorpg_connection_manager PRIVATE
    MMORPG_CONNECTION_MANAGER_EXPORTS
)
