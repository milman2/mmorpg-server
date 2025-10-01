# 소스 디렉토리 CMakeLists.txt

# 공통 라이브러리
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(network)
add_subdirectory(utils)
add_subdirectory(config)

# 에이전트 라이브러리들
add_subdirectory(agents/connection_manager)
add_subdirectory(agents/authentication)
add_subdirectory(agents/game_world)
add_subdirectory(agents/player)
add_subdirectory(agents/combat)
add_subdirectory(agents/chat)
add_subdirectory(agents/guild)
add_subdirectory(agents/economy)
add_subdirectory(agents/database)
add_subdirectory(agents/monitoring)
add_subdirectory(agents/matchmaking)
add_subdirectory(agents/event)
add_subdirectory(agents/analytics)

# 메인 실행 파일
add_executable(mmorpg_server
    main.cpp
)

# 링크할 라이브러리들
target_link_libraries(mmorpg_server
    PRIVATE
    mmorpg_common
    mmorpg_core
    mmorpg_network
    mmorpg_utils
    mmorpg_config
    mmorpg_connection_manager
    mmorpg_authentication
    mmorpg_game_world
    mmorpg_player
    mmorpg_combat
    mmorpg_chat
    mmorpg_guild
    mmorpg_economy
    mmorpg_database
    mmorpg_monitoring
    mmorpg_matchmaking
    mmorpg_event
    mmorpg_analytics
    # 외부 라이브러리
    ${CONAN_LIBS}
)

# 컴파일 정의
target_compile_definitions(mmorpg_server PRIVATE
    MMORPG_VERSION="${PROJECT_VERSION}"
    MMORPG_BUILD_DATE="${CMAKE_BUILD_DATE}"
)
