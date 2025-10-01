# 공통 모듈 라이브러리

add_library(mmorpg_common STATIC
    base_agent.cpp
    message_bus.cpp
    event_system.cpp
    memory_pool.cpp
    lock_free_queue.cpp
    thread_pool.cpp
    logger.cpp
    timer.cpp
    metrics.cpp
)

target_include_directories(mmorpg_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/common
)

target_compile_definitions(mmorpg_common PRIVATE
    MMORPG_COMMON_EXPORTS
)
