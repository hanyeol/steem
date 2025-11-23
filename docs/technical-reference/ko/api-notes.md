# API Notes

## API란 무엇인가

API는 WebSocket / HTTP를 통해 액세스할 수 있고 단일 C++ 클래스에서 제공하는 관련 메서드의 집합입니다. API는 연결별로 존재합니다.

## hello_api Plugin으로 컴파일하기

`hello_api` 예제 plugin을 사용할 것입니다. 예제를 따라하려면 다음 명령을 실행하고 다시 컴파일하여 `hello_api` 예제 plugin을 활성화할 수 있습니다:

```bash
ln -s ../example_plugins/hello_api external_plugins/hello_api
```

## 공개적으로 사용 가능한 API

일부 API는 로그인 없이 클라이언트가 사용할 수 있는 public API입니다. 이러한 API는 구성 파일에서 다음과 같이 지정할 수 있습니다:

```ini
public-api = database_api login_api hello_api_api
```

**참고:** `hello_api`는 plugin의 이름이고 `hello_api_api`는 제공하는 API의 이름입니다. 이 약간 혼란스러운 명명은 이후 버전의 plugin에서 수정될 수 있습니다.

명령줄에서 `--public-api`를 지정할 수도 있습니다.

**서버 구성 참고:** `public-api` 구성을 사용자 정의하는 경우 처음 두 API를 `database_api`와 `login_api`로 설정해야 합니다.

**전문가용:** `database_api`와 `login_api`를 처음 두 API로 권장하는 이유는 많은 클라이언트가 이들이 처음 두 API일 것으로 예상하기 때문입니다. 또한 문자열 API 식별자 기능의 FC 구현은 API ID 1에서 작동하는 `get_api_by_name` 메서드를 사용할 수 있어야 하므로, 문자열 API 식별자를 사용하는 클라이언트(권장)는 클라이언트가 로그인 기능을 사용하지 않더라도 `login_api`가 API ID 1에 있어야 합니다.

## 숫자 API 식별자

API는 public API로 지정된 순서대로 숫자 ID가 할당됩니다. 예를 들어 다음과 같이 HTTP를 통해 `hello_api_api`에 액세스할 수 있습니다:

```bash
curl --data '{"jsonrpc": "2.0", "params": [2, "get_message", []], "id":1, "method":"call"}' http://127.0.0.1:8090/rpc
```

숫자 `2`는 `hello_api_api`에 대한 숫자 API 식별자입니다. 이러한 식별자는 API가 연결에 등록될 때 0부터 시작하여 순차적으로 할당됩니다. 따라서 효과적으로 숫자 API 식별자는 위의 `public-api` 선언에서 `hello_api_api`의 순서에 의해 결정됩니다.

API 1(`login_api`)의 `get_api_by_name` 메서드를 사용하여 이름으로 숫자 ID를 쿼리할 수 있습니다:

```bash
curl --data '{"jsonrpc": "2.0", "params": [1, "get_api_by_name", ["hello_api_api"]], "id":1, "method":"call"}' http://127.0.0.1:8790/rpc
```

## 문자열 API 식별자

숫자 식별자로 API를 참조하는 클라이언트 코드는 서버 측 구성과 조정되어야 합니다. 이것은 불편하므로 API 서버는 다음과 같이 이름으로 API를 식별하는 것도 지원합니다:

```bash
curl --data '{"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":1, "method":"call"}' http://127.0.0.1:8090/rpc
```

API 클라이언트가 이 구문을 사용하여 문자열 식별자로 API를 참조하는 것이 모범 사례로 간주됩니다. 그 이유는 클라이언트가 API의 재번호를 초래하는 서버 측 구성 변경에 대해 견고해지기 때문입니다.

## API 액세스 제어 방법

API를 보안하는 세 가지 방법이 있습니다:

- RPC 서버를 localhost에 바인딩하여 신뢰할 수 있는 머신으로 API 소켓 액세스 제한
- 방화벽 구성으로 신뢰할 수 있는 LAN으로 API 소켓 액세스 제한
- 사용자 이름/비밀번호 인증으로 특정 API에 대한 액세스 제한

Steem 개발자는 다음과 같이 localhost에 바인딩하여 API를 보안하기 위해 이러한 방법 중 첫 번째를 사용하는 것을 권장합니다:

```ini
rpc-endpoint = 127.0.0.1:8090
```

## 특정 API 보안

네트워크 수준에서 API를 보안하는 문제는 노드가 일부 API는 공개하고 다른 API는 비공개로 만들고자 하는 배포 시나리오가 있다는 것입니다. `steemd` 프로세스는 개별 API에 대한 사용자 이름/비밀번호 기반 인증을 포함합니다.

사용자 이름/비밀번호가 직접 전송되므로 사용자 이름과 비밀번호로 인증할 때는 TLS 연결을 사용해야 합니다. TLS 연결은 다음 두 가지 방법 중 하나로 달성할 수 있습니다:

- `rpc-tls-endpoint` 구성
- localhost(또는 보안 LAN)에 바인딩된 `rpc-endpoint`를 구성하고 외부 역방향 프록시(예: `nginx`)를 사용합니다. 이 고급 구성은 여기서 다루지 않습니다.

### SSL 인증서 생성

`.pem` 형식으로 서버용 키와 인증서를 생성해야 합니다. SSL 인증서를 생성하는 많은 튜토리얼이 있으며, 많은 제공업체에서 널리 인정받는 CA가 서명한 SSL 인증서를 생성할 수 있습니다. 작동하는 `openssl` 설치가 있는 경우 다음 명령을 사용하여 자체 서명된 인증서를 생성할 수 있습니다:

```bash
openssl req -x509 -newkey rsa:4096 -keyout data/ss-key.pem -out data/ss-cert.pem -days 365
cat data/ss-{cert,key}.pem > data/ss-cert-key.pem
```

그런 다음 `config.ini`에서 서버에 키와 인증서에 대한 액세스 권한을 부여해야 합니다:

```ini
rpc-tls-endpoint = 0.0.0.0:8091
server-pem = data/ss-cert-key.pem
server-pem-password = password
```

API에 연결할 때 `cli_wallet`을 사용하여 `-a` 옵션으로 특정 인증서, 개별 CA 또는 CA 번들 파일을 인식할 수 있습니다:

```bash
programs/cli_wallet/cli_wallet -s wss://mydomain.tld:8091 -a data/ss-cert.pem
```

**참고:** 호스트 이름 `mydomain.tld`는 인증서의 CN 필드와 일치해야 합니다.

### 예시: 비밀번호로 보호되는 API

이제 사용자 `bytemaster`가 비밀번호 `supersecret`로만 사용할 수 있도록 `hello_api_api` API를 보호해 보겠습니다. 민감한 API를 제공하는 올바른 plugin이 있는지 확인하기 위해 `config.ini` 파일을 수정하고, 민감한 API가 `public-api`에서 액세스할 수 없는지 확인하고, 액세스 정책을 정의하기 위해 `api-user` 정의를 추가합니다:

```ini
enable-plugin = witness account_history hello_api
public-api = database_api login_api
api-user = {"username" : "bytemaster", "password_hash_b64" : "T2k/wMBB9BKyv7X+ngghL+MaoubEuFb6GWvF3qQ9NU0=", "password_salt_b64" : "HqK9mAQCkWU=", "allowed_apis" : ["hello_api_api"]}
```

`password_hash_b64`와 `password_salt_b64`의 값은 `programs/utils/saltpass.py`를 실행하고 원하는 비밀번호를 입력하여 생성됩니다.

### 인증 테스트

사용자 이름/비밀번호 액세스는 로그인이 본질적으로 *상태 저장*이므로 websocket을 통해서만 사용할 수 있습니다. `wscat` 프로그램을 사용하여 로그인 기능을 시연할 수 있습니다:

```bash
wscat -c wss://mydomain.tld:8091
{"jsonrpc": "2.0", "params": ["database_api", "get_dynamic_global_properties", []], "id":1, "method":"call"}
< (ok)
{"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":2, "method":"call"}
< (error)
{"jsonrpc": "2.0", "params": ["login_api", "login", ["bytemaster", "supersecret"]], "id":3, "method":"call"}
< (ok)
{"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":4, "method":"call"}
< (ok)
```

`cli_wallet`도 제한된 API에 대한 액세스를 제공하기 위해 로그인할 수 있는 기능이 있습니다.
