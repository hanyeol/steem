# steemd 시작 프로세스

이 문서는 `steemd` 노드가 시작될 때 실행되는 코드의 흐름과 각 단계에서 일어나는 주요 작업들을 설명합니다.

## 개요

`steemd`는 Steem 블록체인 노드의 메인 데몬 프로그램입니다. 시작 과정은 크게 다음 단계로 구성됩니다:

1. **초기화 (Initialize)** - 설정 파일 파싱, 플러그인 등록 및 초기화
2. **시작 (Startup)** - 데이터베이스 오픈, 플러그인 시작
3. **실행 (Exec)** - 이벤트 루프 실행 및 신호 처리

## 시작 진입점: main()

**파일**: [programs/steemd/main.cpp](../../programs/steemd/main.cpp)

### 주요 단계

#### 1. 프로그램 옵션 설정 (라인 68-79)

```cpp
bpo::options_description options;
steem::utilities::set_logging_program_options( options );
options.add_options()
   ("backtrace", bpo::value< string >()->default_value( "yes" ), "Whether to print backtrace on SIGSEGV" );
appbase::app().add_program_options( bpo::options_description(), options );
```

- 로깅 관련 옵션과 backtrace 옵션을 설정합니다.
- `appbase::app()`을 통해 애플리케이션 싱글톤 인스턴스에 접근합니다.

#### 2. 플러그인 등록 (라인 81)

```cpp
steem::plugins::register_plugins();
```

**파일**: [src/base/manifest/include/steem/manifest/plugins.hpp](../../src/base/manifest/include/steem/manifest/plugins.hpp)

- `register_plugins()` 함수가 모든 사용 가능한 플러그인을 `appbase::application`에 등록합니다.
- 이 함수는 빌드 시스템에 의해 자동 생성되며, `src/plugins/` 디렉토리의 모든 플러그인을 포함합니다.
- 각 플러그인은 `appbase::app().register_plugin<PluginType>()` 형태로 등록됩니다.

#### 3. 애플리케이션 메타데이터 설정 (라인 83-84)

```cpp
appbase::app().set_version_string( version_string() );
appbase::app().set_app_name( "steemd" );
```

#### 4. 기본 플러그인 설정 (라인 86-91)

```cpp
appbase::app().set_default_plugins<
   steem::plugins::witness::witness_plugin,
   steem::plugins::account_by_key::account_by_key_plugin,
   steem::plugins::account_by_key::account_by_key_api_plugin >();
```

- 설정 파일에 명시하지 않아도 기본적으로 활성화될 플러그인들을 지정합니다.
- 이 플러그인들은 `config.ini` 파일 생성 시 기본값으로 포함됩니다.

#### 5. 필수 플러그인 초기화 (라인 94-98)

```cpp
bool initialized = appbase::app().initialize<
      steem::plugins::chain::chain_plugin,
      steem::plugins::p2p::p2p_plugin,
      steem::plugins::webserver::webserver_plugin >
      ( argc, argv );
```

**파일**: [src/base/appbase/include/appbase/application.hpp](../../src/base/appbase/include/appbase/application.hpp#L37)

- 설정 파일 내용과 관계없이 **항상 로드되는 필수 플러그인**들입니다:
  - `chain_plugin`: 블록체인 데이터베이스 관리
  - `p2p_plugin`: 피어 투 피어 네트워크
  - `webserver_plugin`: RPC 웹서버

#### 6. 로깅 설정 (라인 109-116)

```cpp
fc::optional< fc::logging_config > logging_config =
   steem::utilities::load_logging_config( args, appbase::app().data_dir() );
if( logging_config )
   fc::configure_logging( *logging_config );
```

- `logging.json` 파일이 있으면 로드하여 로깅을 설정합니다.

#### 7. Backtrace 설정 (라인 118-122)

```cpp
if( args.at( "backtrace" ).as< string >() == "yes" )
{
   fc::print_stacktrace_on_segfault();
   ilog( "Backtrace on segfault is enabled." );
}
```

- SIGSEGV 발생 시 스택 추적을 출력하도록 설정합니다.

#### 8. 애플리케이션 시작 및 실행 (라인 124-125)

```cpp
appbase::app().startup();
appbase::app().exec();
```

## appbase::application 초기화 프로세스

**파일**: [src/base/appbase/application.cpp](../../src/base/appbase/application.cpp)

### initialize_impl() (라인 93-190)

이 함수는 `initialize<>()` 템플릿 메서드에서 호출되며 다음 작업을 수행합니다:

#### 1. 프로그램 옵션 파싱 (라인 97-98)

```cpp
set_program_options();
bpo::store( bpo::parse_command_line( argc, argv, my->_app_options ), my->_args );
```

- 모든 등록된 플러그인의 커맨드라인 옵션을 수집하여 파싱합니다.

#### 2. 도움말 및 버전 확인 (라인 100-109)

- `--help` 또는 `--version` 플래그가 있으면 정보를 출력하고 `false`를 반환합니다.

#### 3. 데이터 디렉토리 설정 (라인 111-150)

```cpp
bfs::path data_dir;
if( my->_args.count("data-dir") )
{
   data_dir = my->_args["data-dir"].as<bfs::path>();
   // ...
}
else
{
   // 기본값: $HOME/.steemd 또는 현재 디렉토리/.steemd
}
```

- 기본 데이터 디렉토리는 `$HOME/.steemd` (Windows는 `%APPDATA%/.steemd`)입니다.
- `--data-dir` 옵션으로 변경 가능합니다.

#### 4. 설정 파일 로드 (라인 152-164)

```cpp
bfs::path config_file_name = data_dir / "config.ini";
if(!bfs::exists(config_file_name)) {
   write_default_config(config_file_name);
}
bpo::store(bpo::parse_config_file< char >(
   config_file_name.make_preferred().string().c_str(),
   my->_cfg_options, true ), my->_args );
```

- `config.ini` 파일이 없으면 기본 설정으로 생성합니다.
- 설정 파일을 파싱하여 옵션을 로드합니다.

#### 5. 플러그인 초기화 (라인 166-179)

```cpp
if(my->_args.count("plugin") > 0)
{
   auto plugins = my->_args.at("plugin").as<std::vector<std::string>>();
   for(auto& arg : plugins)
      get_plugin(name).initialize(my->_args);
}
for (const auto& plugin : autostart_plugins)
   if (plugin != nullptr && plugin->get_state() == abstract_plugin::registered)
      plugin->initialize(my->_args);
```

- `config.ini`에 지정된 플러그인들을 초기화합니다.
- `initialize<>`에 템플릿 인자로 전달된 필수 플러그인들을 초기화합니다.

**플러그인 초기화 순서**:
1. 플러그인의 의존성 플러그인들을 먼저 초기화 (재귀적)
2. `plugin_initialize()` 메서드 호출
3. 초기화된 플러그인 목록에 추가

**파일**: [src/base/appbase/include/appbase/application.hpp](../../src/base/appbase/include/appbase/application.hpp#L153-165)

### startup() (라인 36-39)

```cpp
void application::startup() {
   for (const auto& plugin : initialized_plugins)
      plugin->startup();
}
```

- 초기화된 모든 플러그인의 `startup()` 메서드를 순서대로 호출합니다.
- 각 플러그인은 의존성 플러그인들이 먼저 시작된 후 자신의 `plugin_startup()` 메서드를 실행합니다.

### exec() (라인 210-231)

```cpp
void application::exec() {
   signal(SIGPIPE, SIG_IGN);

   std::shared_ptr<boost::asio::signal_set> sigint_set(
      new boost::asio::signal_set(*io_serv, SIGINT));
   sigint_set->async_wait([sigint_set,this](...) {
     quit();
     sigint_set->cancel();
   });

   std::shared_ptr<boost::asio::signal_set> sigterm_set(
      new boost::asio::signal_set(*io_serv, SIGTERM));
   sigterm_set->async_wait([sigterm_set,this](...) {
     quit();
     sigterm_set->cancel();
   });

   io_serv->run();

   shutdown();
}
```

- SIGINT (Ctrl+C) 및 SIGTERM 신호 핸들러를 등록합니다.
- Boost.Asio I/O 서비스 이벤트 루프를 실행합니다.
- 종료 신호를 받으면 I/O 서비스를 중지하고 `shutdown()`을 호출합니다.

## 주요 플러그인의 초기화 및 시작

### 1. chain_plugin

**파일**: [src/plugins/chain/chain_plugin.cpp](../../src/plugins/chain/chain_plugin.cpp)

#### plugin_initialize() (라인 323-379)

설정 옵션을 파싱하고 저장합니다:

- `shared-file-dir`: 블록체인 공유 메모리 파일 위치 (기본: `blockchain`)
- `shared-file-size`: 공유 메모리 파일 크기 (기본: 24GB, 풀노드는 200GB 권장)
- `replay-blockchain`: 체인 데이터베이스를 지우고 모든 블록을 재생
- `resync-blockchain`: 체인 데이터베이스와 블록 로그 모두 삭제
- `checkpoint`: 체크포인트 블록 설정
- `flush-state-interval`: 공유 메모리 변경사항을 디스크에 플러시하는 블록 간격

#### plugin_startup() (라인 383-528)

블록체인 데이터베이스를 초기화하고 시작합니다:

**1. 데이터베이스 오픈 인자 설정 (라인 426-435)**

```cpp
database::open_args db_open_args;
db_open_args.data_dir = app().data_dir() / "blockchain";
db_open_args.shared_mem_dir = my->shared_memory_dir;
db_open_args.initial_supply = STEEM_INIT_SUPPLY;
db_open_args.shared_file_size = my->shared_memory_size;
db_open_args.shared_file_full_threshold = my->shared_file_full_threshold;
db_open_args.shared_file_scale_rate = my->shared_file_scale_rate;
db_open_args.do_validate_invariants = my->validate_invariants;
```

**2. 리플레이 또는 정상 오픈 (라인 470-522)**

```cpp
if(my->replay)
{
   ilog("Replaying blockchain on user request.");
   last_block_number = my->db.reindex( db_open_args );
}
else
{
   try {
      my->db.open( db_open_args );
   }
   catch( const fc::exception& e ) {
      wlog("Error opening database, attempting to replay blockchain.");
      my->db.reindex( db_open_args );
   }
}
```

- `--replay-blockchain` 플래그가 있으면 블록 로그로부터 모든 블록을 재생합니다.
- 정상 오픈 실패 시 자동으로 리플레이를 시도합니다.

**3. 쓰기 처리 스레드 시작 (라인 527)**

```cpp
my->start_write_processing();
```

- 별도의 스레드에서 블록 및 트랜잭션 쓰기 작업을 처리합니다.
- [chain_plugin.cpp:197-268](../../src/plugins/chain/chain_plugin.cpp#L197-268) 참조

### 2. p2p_plugin

**파일**: [src/plugins/p2p/p2p_plugin.cpp](../../src/plugins/p2p/p2p_plugin.cpp)

P2P 네트워크 레이어를 관리합니다:

**plugin_initialize()**:
- 시드 노드 설정
- P2P 엔드포인트 설정
- 최대 연결 수 설정

**plugin_startup()**:
- Graphene 네트워크 노드 초기화
- 피어 연결 시작
- 블록 및 트랜잭션 동기화 시작

주요 설정:
- `p2p-endpoint`: 다른 피어가 연결할 엔드포인트 (예: `0.0.0.0:2001`)
- `p2p-seed-node`: 초기 연결할 시드 노드들
- `p2p-max-connections`: 최대 피어 연결 수

### 3. webserver_plugin

**파일**: [src/plugins/webserver/webserver_plugin.cpp](../../src/plugins/webserver/webserver_plugin.cpp)

HTTP/WebSocket RPC 서버를 제공합니다:

**plugin_initialize()**:
- HTTP 및 WebSocket 엔드포인트 설정
- CORS 설정

**plugin_startup()**:
- HTTP 서버 시작
- WebSocket 서버 시작
- API 핸들러 등록

주요 설정:
- `webserver-http-endpoint`: HTTP RPC 엔드포인트 (예: `127.0.0.1:8090`)
- `webserver-ws-endpoint`: WebSocket RPC 엔드포인트 (예: `127.0.0.1:8091`)
- `rpc-endpoint`: 레거시 설정 (webserver-http-endpoint와 동일)

## 플러그인 생명주기 및 의존성

### 플러그인 상태

**파일**: [src/base/appbase/include/appbase/plugin.hpp](../../src/base/appbase/include/appbase/plugin.hpp)

각 플러그인은 다음 상태를 가집니다:

1. **registered**: 플러그인이 등록되었지만 아직 초기화되지 않음
2. **initialized**: `plugin_initialize()`가 호출되어 초기화됨
3. **started**: `plugin_startup()`이 호출되어 시작됨
4. **stopped**: `plugin_shutdown()`이 호출되어 종료됨

### 의존성 처리

플러그인은 다른 플러그인에 의존할 수 있습니다. 의존성은 다음과 같이 선언됩니다:

```cpp
class my_plugin : public appbase::plugin< my_plugin >
{
   // plugin_for_each_dependency를 오버라이드하여 의존성 선언
   template< typename Lambda >
   void plugin_for_each_dependency( Lambda&& l )
   {
      l( app().get_plugin< chain_plugin >() );
      l( app().get_plugin< p2p_plugin >() );
   }
};
```

**초기화 순서 보장**:
- 플러그인이 초기화될 때, 먼저 모든 의존성 플러그인이 초기화됩니다 (재귀적).
- 플러그인이 시작될 때, 먼저 모든 의존성 플러그인이 시작됩니다.
- 플러그인이 종료될 때는 **역순**으로 종료됩니다.

## 종료 프로세스

### shutdown() (라인 192-204)

```cpp
void application::shutdown() {
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      (*ritr)->shutdown();
   }
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      plugins.erase((*ritr)->get_name());
   }
   running_plugins.clear();
   initialized_plugins.clear();
   plugins.clear();
}
```

- 시작 순서의 **역순**으로 플러그인들을 종료합니다.
- 각 플러그인의 `plugin_shutdown()` 메서드를 호출합니다.
- 모든 플러그인 참조를 정리합니다.

## 데이터 디렉토리 구조

기본 데이터 디렉토리는 `$HOME/.steemd`이며 다음과 같은 구조를 가집니다:

```
.steemd/
├── config.ini              # 설정 파일
├── logging.json            # 로깅 설정 (선택적)
├── blockchain/             # 블록체인 데이터
│   ├── block_log           # 불변 블록 로그
│   ├── block_log.index     # 블록 로그 인덱스
│   └── shared_memory.bin   # 공유 메모리 상태 파일 (24GB+)
└── p2p/                    # P2P 노드 데이터 (선택적)
    └── peers.json          # 피어 정보
```

## 주요 설정 파일: config.ini

첫 실행 시 자동으로 생성되며 다음 주요 섹션을 포함합니다:

### 플러그인 설정
```ini
plugin = witness account_by_key account_by_key_api
```

### chain_plugin 설정
```ini
shared-file-dir = blockchain
shared-file-size = 24G
```

### p2p_plugin 설정
```ini
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed-1.steemit.com:2001
```

### webserver_plugin 설정
```ini
webserver-http-endpoint = 127.0.0.1:8090
webserver-ws-endpoint = 127.0.0.1:8091
```

## 시작 시 일반적인 로그 메시지

```
------------------------------------------------------

            STARTING STEEM NETWORK

------------------------------------------------------
genesis public key: STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX
chain id: 0000000000000000000000000000000000000000000000000000000000000000
blockchain version: 0.23.0
------------------------------------------------------
Opening shared memory from /home/user/.steemd/blockchain
Starting chain with shared_file_size: 25769803776 bytes
Started on blockchain with 12345678 blocks
```

## 디버깅 및 문제 해결

### 로그 레벨 조정

`logging.json` 파일을 수정하여 로그 레벨을 조정할 수 있습니다:

```json
{
  "appenders": [{
    "name": "stderr",
    "type": "console",
    "stream": "std_error",
    "level_colors": [{
        "level": "debug",
        "color": "green"
      }]
  }],
  "loggers": [{
    "name": "default",
    "level": "debug",
    "appenders": ["stderr"]
  }]
}
```

### 체인 재생

데이터베이스가 손상되었거나 플러그인 구성이 변경된 경우:

```bash
steemd --replay-blockchain
```

### 전체 재동기화

블록 로그부터 완전히 다시 시작:

```bash
steemd --resync-blockchain
```

## 관련 파일

- [programs/steemd/main.cpp](../../programs/steemd/main.cpp) - 메인 진입점
- [src/base/appbase/application.cpp](../../src/base/appbase/application.cpp) - 애플리케이션 프레임워크
- [src/base/appbase/include/appbase/application.hpp](../../src/base/appbase/include/appbase/application.hpp) - 애플리케이션 인터페이스
- [src/plugins/chain/chain_plugin.cpp](../../src/plugins/chain/chain_plugin.cpp) - 체인 플러그인
- [src/plugins/p2p/p2p_plugin.cpp](../../src/plugins/p2p/p2p_plugin.cpp) - P2P 플러그인
- [src/plugins/webserver/webserver_plugin.cpp](../../src/plugins/webserver/webserver_plugin.cpp) - 웹서버 플러그인

## 참고 문서

- [Plugin Development Guide](plugin.md) - 플러그인 개발 가이드
- [Building Guide](../getting-started/building.md) - 빌드 가이드
- [Node Modes Guide](../operations/node-modes-guide.md) - 노드 타입 가이드
