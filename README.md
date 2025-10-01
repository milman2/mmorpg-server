# MMORPG Server (C++)

5000명 동시 접속을 지원하는 고성능 C++ MMORPG 게임 서버입니다.

## 🚀 주요 특징

- **고성능**: C++ 기반으로 < 50ms PvP 지연시간 달성
- **확장성**: 5,000명 동시 접속 지원
- **모듈화**: 에이전트 기반 마이크로서비스 아키텍처
- **최적화**: Lock-free 프로그래밍, SIMD, 커스텀 메모리 풀

## 📋 요구사항

### 시스템 요구사항
- **OS**: Ubuntu 20.04+ / CentOS 8+ / Windows 10+
- **CPU**: x86_64, AVX2 지원 권장
- **RAM**: 최소 8GB, 권장 16GB+
- **네트워크**: 1Gbps+ 대역폭

### 개발 도구
- **컴파일러**: GCC 11+ / Clang 12+ / MSVC 2019+
- **CMake**: 3.20+
- **Conan**: 2.0+
- **Docker**: 20.10+ (컨테이너 배포용)

## 🛠️ 빌드 및 실행

### 1. 의존성 설치

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git

# Conan 설치
pip install conan
conan profile detect --force
```

### 2. 프로젝트 빌드

```bash
# 전체 빌드
./scripts/build.sh Release

# 디버그 빌드
./scripts/build.sh Debug

# 빌드 + 테스트
./scripts/build.sh Release --test
```

### 3. 서버 실행

```bash
# 개발 환경
./build_release/bin/mmorpg_server

# 프로덕션 환경
./build_release/bin/mmorpg_server --config=config/production/server.conf
```

## 🏗️ 아키텍처

### 핵심 에이전트

1. **Connection Manager**: 5,000 동시 연결 관리
2. **Authentication**: JWT 기반 인증 시스템
3. **Game World**: 월드 상태 및 물리 엔진
4. **Player**: 플레이어 상태 및 인벤토리
5. **Combat**: 실시간 전투 시스템 (< 10ms 응답)
6. **Chat**: 실시간 채팅 (10,000 msg/s)
7. **Guild**: 길드 시스템 및 그룹 관리
8. **Economy**: 거래소 및 경제 시스템
9. **Database**: 분산 데이터베이스 관리
10. **Monitoring**: 실시간 모니터링

### 기술 스택

- **언어**: C++20
- **네트워킹**: Boost.Asio
- **물리 엔진**: Bullet Physics
- **암호화**: OpenSSL
- **로깅**: spdlog
- **JSON**: RapidJSON
- **빌드**: CMake + Conan

## 📊 성능 목표

| 항목 | 목표 | 현재 |
|------|------|------|
| 동시 접속자 | 5,000명 | - |
| PvP 지연시간 | < 50ms | - |
| PvE 지연시간 | < 100ms | - |
| 전투 응답시간 | < 10ms | - |
| 메시지 처리량 | 10,000 msg/s | - |
| 가용성 | 99.9% | - |

## 🧪 테스트

```bash
# 단위 테스트
cd build_release
ctest --output-on-failure

# 성능 테스트
./tools/benchmark/performance_test

# 부하 테스트
./tools/load_test/load_test --connections=5000 --duration=300s
```

## 📈 모니터링

### 메트릭 수집
- **Prometheus**: 메트릭 수집
- **Grafana**: 대시보드 시각화
- **Jaeger**: 분산 트레이싱

### 로그 관리
- **spdlog**: 구조화된 로깅
- **ELK Stack**: 로그 수집 및 분석

## 🐳 Docker 배포

```bash
# 이미지 빌드
docker build -t mmorpg-server:latest -f docker/Dockerfile .

# 컨테이너 실행
docker run -d \
  --name mmorpg-server \
  -p 8000-8010:8000-8010 \
  -v $(pwd)/config:/app/config \
  mmorpg-server:latest

# Docker Compose 실행
docker-compose up -d
```

## 🔧 설정

### 환경 변수
```bash
# 데이터베이스
export DATABASE_URL="postgresql://user:pass@localhost:5432/mmorpg"

# Redis
export REDIS_URL="redis://localhost:6379"

# JWT
export JWT_SECRET_KEY="your-secret-key"
```

### 설정 파일
```ini
# config/development/server.conf
[server]
max_connections = 5000
tick_rate = 60
worker_threads = 8

[database]
host = localhost
port = 5432
database = mmorpg
```

## 📚 문서

- [API 문서](docs/api/)
- [설계 문서](docs/design/)
- [배포 가이드](docs/deployment/)

## 🤝 기여하기

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 📞 지원

- **이슈**: [GitHub Issues](https://github.com/your-repo/issues)
- **토론**: [GitHub Discussions](https://github.com/your-repo/discussions)
- **이메일**: support@yourgame.com