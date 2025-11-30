# Debug Node Plugin

## 개요

최신 릴리스는 플러그인 아키텍처를 노출합니다. 각 플러그인은 데이터베이스에 대한 전체 액세스 권한을 가지며 노드 동작을 변경할 수 있습니다. 각 플러그인은 API도 제공할 수 있습니다.

## debug_node Plugin

`debug_node` 플러그인의 목표는 라이브 체인에서 시작한 다음 미래의 가상 작업을 쉽게 시뮬레이션하는 것입니다. 플러그인은 체인 상태 변경을 시뮬레이션합니다. 예를 들어, 계정의 잔액과 서명 키를 편집하여 해당 계정으로 (시뮬레이션된) 작업을 수행할 수 있도록 할 수 있습니다.

## 왜 이것이 안전하지 않은 것이 아닌가

누구든지 (당신도!) 자신의 계정을 편집하여 1백만 STEEM을 포함하도록 할 수 있습니다. 개발자가 물리적으로 당신이 자신의 컴퓨터의 메모리/디스크를 편집하여 로컬 노드가 어떤 계정 잔액이든 어떤 금액이든 생각하도록 만들 수 없게 막는 것은 정말 불가능하므로, 당신은 자신에게 1백만 STEEM을 "줄" 수 있습니다. 마치 은행이 물리적으로 당신이 수표장 잔액을 맞출 때 원하는 숫자를 쓰는 것을 막을 수 없는 것처럼, 당신은 자신에게 1백만 달러를 "줄" 수 있습니다.

그러나 다른 노드가 무엇을 하는지 (또는 은행의 직원과 컴퓨터 시스템이 무엇을 하는지) 제어할 방법이 없습니다. 그들은 자신의 장부를 관리하고 당신의 실제 잔액이 무엇인지 추적합니다 (당신이 자신에게 "준" 모든 가짜 STEEM이나 가짜 달러 없이). 따라서 당신은 자신의 잔액에 대해 원하는 것을 믿고 자신의 장부 시스템의 규칙을 다시 작성하여 원하는 잔액을 보여줄 수 있지만, 실제로 가지고 있지 않은 STEEM (또는 달러)을 지출하려고 하면 즉시 중단됩니다. 네트워크의 다른 모든 노드는 당신이 제어하지 않는 시스템으로, 장부를 적절하게 관리하고 (추가 자금을 주기 위한 모든 편집 없이), 모든 트랜잭션에 대해 자체 검증을 수행하며 충분한 잔액과 적절한 암호화 서명이 없는 트랜잭션을 억제합니다.

## 디버깅에 유용한 이유

하드포크를 위한 코드를 작성했다고 가정해 봅시다 -- 특정 시간 또는 특정 블록 번호에 모든 노드에서 활성화되는 새로운 기능 또는 버그 수정 -- 예를 들어 1월 1일 자정 (가상 날짜). 묻는 기본적인 질문은, 현재 체인에 해당 코드를 적용하면 1월 1일에 블록 생성이 예상대로 계속되는가입니다. 프로세스가 segfault하거나, 블록 생성이 중단되거나, 1월 1일 이후 아무도 트랜잭션을 수행할 수 없다면, 그것들은 확실히 코드가 릴리스에 포함될 준비가 되기 전에 수정해야 하는 치명적인 문제입니다. 따라서 테스트하려면 시간을 빠르게 진행하고 1월 1일까지 블록 생성을 시뮬레이션하는 것이 좋지만, 증인 개인 키가 없습니다. `debug_node_plugin`을 사용하면 증인 키를 자신이 제어하는 키로 변경하는 것을 시뮬레이션하고 원하는 만큼 블록을 생성할 수 있습니다.

## 사용 예

### Step 1: 블록 저장

체인의 히스토리를 얻고 싶다고 가정해 봅시다. 첫 번째 작업은 실행 중인 노드에서 블록을 저장하는 것입니다:

```bash
cp -Rp datadir/blockchain/database/block_num_to_block /mydir/myblocks
```

### Step 2: Debug Node 구성

그런 다음 빈 datadir와 시드 노드가 없는 새 노드를 만듭니다. (시드 노드가 없다는 것은 p2p 네트워크에 참여하지 않으며 나중에 명시적으로 로드하도록 지시할 블록을 제외하고는 블록을 수신하지 않는다는 것을 의미합니다.) debug node를 활성화하고 누구에게나 API에 대한 액세스를 허용합니다:

```ini
# config 파일이나 명령줄에 seed-node 없음
p2p-endpoint = 127.0.0.1:2001       # localhost에 바인딩하여 원격 p2p 노드가 연결하지 못하도록 함
rpc-endpoint = 127.0.0.1:8090       # localhost에 바인딩하여 RPC API 액세스 보안
enable-plugin = witness account_history debug_node
public-api = database_api login_api debug_node_api
```

**보안 참고**: `public-api` 섹션은 RPC 엔드포인트에 액세스할 수 있는 API를 나열합니다. (위에서 설명한 것처럼) `debug_node_api`는 데이터베이스를 편집하여 노드가 클라이언트에게 시뮬레이션된 체인 상태를 보고하고 실제 네트워크와 동기화하지 못하도록 할 수 있으므로, `debug_node_api`가 공개 API 목록에 있을 때 RPC는 전체 인터넷에 액세스할 수 없어야 합니다.

### API 메서드

`public-api`로 구성된 API는 0부터 시작하는 번호가 할당됩니다. 따라서 `debug_node_api`는 API 번호 2로 호출할 수 있습니다.

API는 다음 메서드를 제공합니다 (이러한 정의는 `src/plugins/debug_node/include/steem/plugins/debug_node/debug_node_api.hpp` 참조):

```cpp
void debug_push_blocks( std::string src_filename, uint32_t count );
void debug_generate_blocks( std::string debug_key, uint32_t count );
void debug_update_object( fc::variant_object update );
void debug_stream_json_objects( std::string filename );
void debug_stream_json_objects_flush();
```

### Step 3: 블록 로드

자, `steemd`를 실행해 봅시다. 블록 없이 즉시 시작해야 합니다. 이전에 저장한 디렉토리에서 블록을 읽도록 요청할 수 있습니다:

```bash
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_push_blocks",["/mydir/myblocks", 1000]], "id": 1}' http://127.0.0.1:8090/rpc
```

예상대로 이제 1000개의 블록이 있습니다:

```bash
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_dynamic_global_properties",[]], "id": 2}' http://127.0.0.1:8090/rpc
```

현재 헤드 블록 이후의 블록에 대해 디렉토리를 쿼리하므로 이후에 500개를 더 로드할 수 있습니다:

```bash
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_push_blocks",["/mydir/myblocks", 500]], "id": 3}' http://127.0.0.1:8090/rpc
```

### Step 4: 블록 생성

몇 개의 블록을 생성해 봅시다:

```bash
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_generate_blocks",["5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3",5]], "id": 4}' http://127.0.0.1:8090/rpc
```

볼 수 있듯이 `debug_node`는 각 증인의 공개 키를 로컬에서 편집합니다:

```bash
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["dantheman4"]], "id": 5}' http://127.0.0.1:8990/rpc
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["thisisnice4"]], "id": 6}' http://127.0.0.1:8990/rpc
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_dynamic_global_properties",[]], "id": 7}' http://127.0.0.1:8090/rpc
```

위의 중요한 정보는 다음과 같습니다:

```
... "signing_key":"STM6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" ...
... "head_block_number":1505 ...
```

이것은 증인 키가 재설정되었고 헤드 블록 번호가 새 블록으로 진행되었음을 보여줍니다. 블록은 위의 개인 키로 서명되었으며, 데이터베이스는 예정된 증인의 블록 서명 키를 적절하게 설정하도록 편집되어 노드가 시뮬레이션된 서명을 유효한 것으로 수락합니다.

### Step 5: 계정 제어하기

계정을 제어하려면 다음과 같이 `debug_update_object` 명령으로 키를 편집할 수 있습니다:

```bash
# 원하는 계정의 ID가 2.2.28인지 확인
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"lookup_account_names",[["steemit"]]], "id": 8}' http://127.0.0.1:8090/rpc

# 계정의 키 업데이트
curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_update_object",[{"_action":"update","id":"2.2.28","active":{"weight_threshold":1,"key_auths":[["STM6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",1]]}}]], "id": 9}]' http://127.0.0.1:8090/rpc
```

이제 키를 재설정했으므로 지갑에서 제어할 수 있습니다:

```bash
programs/cli_wallet/cli_wallet -w debug_wallet.json -s ws://127.0.0.1:8090
```

```
set_password abc
unlock abc
import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
list_my_accounts
transfer steemit dantheman "1.234 STEEM" "make -j100 money" true
list_my_accounts
get_account_history steemit -1 1000
```

## 중요 참고사항

다시 말하지만, 우리는 실제로 무언가를 제어하는 것이 아니라 "what-if" 실험을 하고 있습니다 -- 이 계정의 키가 블록 1505에서 갑자기 변경되고 그런 다음 이 전송 operation이 브로드캐스트되었다면 어떻게 될까요? 그리고 우리는 체인과 지갑이 예상대로 상황을 처리한다는 것을 알게 됩니다 (전송 처리, 계정 히스토리에 넣기, 잔액 업데이트).

이 플러그인은 체인과의 모든 종류의 창의적인 "what-if" 실험을 허용합니다.
