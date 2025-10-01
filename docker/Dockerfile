# Multi-stage build for MMORPG Server
FROM conanio/gcc11-ubuntu20.04:latest AS builder

# 작업 디렉토리 설정
WORKDIR /app

# 시스템 패키지 업데이트 및 필요한 도구 설치
RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

# 소스 코드 복사
COPY . .

# Conan 의존성 설치
RUN conan install . --build=missing --settings=build_type=Release

# 빌드 실행
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../conan_toolchain.cmake && \
    cmake --build . --config Release --parallel $(nproc)

# Runtime stage
FROM ubuntu:20.04

# 런타임 의존성 설치
RUN apt-get update && apt-get install -y \
    libssl1.1 \
    libpq5 \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 빌드된 바이너리 복사
COPY --from=builder /app/build/bin/mmorpg_server /app/
COPY --from=builder /app/config /app/config

# 포트 노출
EXPOSE 8000-8010

# 헬스체크 설정
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8000/health || exit 1

# 실행 명령
CMD ["./mmorpg_server"]
