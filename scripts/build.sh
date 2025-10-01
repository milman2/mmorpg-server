#!/bin/bash

# MMORPG Server 빌드 스크립트

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 함수 정의
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 빌드 타입 설정 (기본값: Release)
BUILD_TYPE=${1:-Release}
BUILD_DIR="build_${BUILD_TYPE,,}"

print_info "MMORPG Server 빌드 시작"
print_info "빌드 타입: $BUILD_TYPE"
print_info "빌드 디렉토리: $BUILD_DIR"

# 이전 빌드 디렉토리 정리
if [ -d "$BUILD_DIR" ]; then
    print_warning "이전 빌드 디렉토리 제거 중..."
    rm -rf "$BUILD_DIR"
fi

# 빌드 디렉토리 생성
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Conan 의존성 설치
print_info "Conan 의존성 설치 중..."
conan install .. --build=missing --settings=build_type=$BUILD_TYPE

# CMake 설정
print_info "CMake 설정 중..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DENABLE_SIMD=ON \
    -DENABLE_PROFILING=OFF

# 빌드 실행
print_info "빌드 실행 중..."
cmake --build . --config $BUILD_TYPE --parallel $(nproc)

# 빌드 성공 메시지
print_info "빌드 완료!"
print_info "실행 파일 위치: $BUILD_DIR/bin/mmorpg_server"

# 성능 테스트 실행 (선택사항)
if [ "$2" = "--test" ]; then
    print_info "성능 테스트 실행 중..."
    ctest --output-on-failure
fi

print_info "빌드 스크립트 완료"
