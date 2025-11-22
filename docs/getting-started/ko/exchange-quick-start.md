# 거래소 빠른 시작
-------------------

시스템 요구사항: 최소 16GB RAM을 가진 전용 서버 또는 가상 머신, 그리고 최소 50GB의 빠른 로컬 SSD 스토리지. STEEM은 세계에서 가장 활발한 블록체인 중 하나이며 초당 엄청나게 많은 트랜잭션을 처리하므로 효율적으로 실행하려면 빠른 스토리지가 필요합니다.

거래소의 경우 STEEM을 빌드하고 실행하는 데 docker를 사용하는 것이 좋습니다. Docker는 세계 최고의 컨테이너화 플랫폼이며, 이를 사용하면 빌드 및 실행 환경이 개발자가 사용하는 것과 동일하다는 것을 보장합니다. 여전히 소스에서 빌드할 수 있으며 블록체인 데이터와 월렛 데이터를 모두 docker 컨테이너 외부에 보관할 수 있습니다. 아래 지침은 몇 가지 간단한 단계로 이를 수행하는 방법을 보여줍니다.

### docker와 git 설치 (아직 설치되지 않은 경우)

Ubuntu 16.04+:
```
apt-get update && apt-get install git docker.io -y
```

다른 배포판에서는 네이티브 패키지 관리자를 사용하거나 get.docker.com의 스크립트를 사용하여 docker를 설치할 수 있습니다:
```
curl -fsSL get.docker.com -o get-docker.sh
sh get-docker.sh
```

### steem 저장소 복제

GitHub의 공식 소스에서 steem 저장소를 가져온 다음 생성된 디렉터리로 이동합니다.
```
git clone https://github.com/steemit/steem
cd steem
```

### docker로 소스에서 이미지 빌드

Docker는 이미 빌드된 이미지를 다운로드하는 것만을 위한 것이 아니라 소스에서 빌드하는 데 사용할 수 있습니다. 이렇게 하면 빌드 환경이 소프트웨어 개발에 사용하는 것과 동일하다는 것을 보장합니다. 아래 명령을 사용하여 빌드를 시작하세요:

```
docker build -t=steemit/steem .
```

현재 디렉터리가 빌드 대상임을 나타내는 줄 끝의 `.`을 잊지 마세요.

이것은 빌드 프로세스 중에 전체 테스트 스위트를 실행하는 것을 포함하여 모든 것을 빌드합니다. 장비의 속도에 따라 30분에서 몇 시간이 걸립니다.

빌드가 완료되면 '성공적으로 빌드됨'을 나타내는 메시지가 표시됩니다.

### 소스에서 빌드하지 않고 공식 Docker 이미지 사용

미리 빌드된 공식 바이너리 이미지를 사용하려면 하나의 명령으로 Dockerhub 레지스트리에서 다운로드하면 됩니다:

```
docker pull steemit/steem
```

### Docker 컨테이너 없이 바이너리 빌드 실행

Docker로 빌드했지만 docker 컨테이너 내에서 steemd를 실행하고 싶지 않다면, 이 단계에서 중지하고 아래 명령으로 컨테이너에서 바이너리를 추출할 수 있습니다. docker로 steemd를 실행할 예정이라면(권장 방법) 이 단계를 완전히 건너뛰세요. 우리는 모든 사람의 사용 사례에 대한 옵션을 제공하고 있을 뿐입니다. 바이너리는 대부분 정적으로 빌드되며 Linux 커널 라이브러리에만 동적으로 링크됩니다. Docker에서 빌드된 바이너리가 Ubuntu와 Fedora에서 작동하는지 테스트하고 확인했으며 많은 다른 Linux 배포판에서도 작동할 것입니다. 직접 이미지를 빌드하거나 미리 빌드된 이미지 중 하나를 가져오는 것 모두 작동합니다.

바이너리를 추출하려면 컨테이너를 시작한 다음 파일을 복사해야 합니다.

```
docker run -d --name steemd-exchange steemit/steem
docker cp steemd-exchange:/usr/local/steemd-default/bin/steemd /local/path/to/steemd
docker cp steemd-exchange:/usr/local/steemd-default/bin/cli_wallet /local/path/to/cli_wallet
docker stop steemd-exchange
```

편의를 위해 거래소 노드를 실행하기에 충분할 것으로 예상되는 [example\_config](example\_config.ini)를 제공했습니다. 이름을 간단히 `config.ini`로 변경하세요.

### Docker 외부에 블록체인 및 월렛 데이터를 저장할 디렉터리 생성

재사용성을 위해 블록체인 및 월렛 데이터를 저장할 디렉터리를 만들고 docker 컨테이너 내부에 쉽게 링크할 수 있습니다.

```
mkdir blockchain
mkdir steemwallet
```

### 컨테이너 실행

아래 명령은 p2p 및 RPC용 포트를 열면서 데몬화된 인스턴스를 시작하고 블록체인 및 월렛 데이터용으로 생성한 디렉터리를 컨테이너 내부에 링크합니다. `TRACK_ACCOUNT`를 팔로우하려는 거래소 계정 이름으로 채우세요. `-v` 플래그는 컨테이너 외부 디렉터리를 내부로 매핑하는 방법이며, 각 `-v` 플래그에 대해 `:`앞에 앞서 생성한 디렉터리의 경로를 나열합니다. 재시작 정책은 시스템이 재시작되더라도 컨테이너가 자동으로 재시작되도록 합니다.

```
docker run -d --name steemd-exchange --env TRACK_ACCOUNT=nameofaccount -p 2001:2001 -p 8090:8090 -v /path/to/steemwallet:/var/steemwallet -v /path/to/blockchain:/var/lib/steemd/blockchain --restart always steemit/steem
```

`docker ps` 명령으로 컨테이너가 실행 중인지 확인할 수 있습니다.

로그를 따라가려면 `docker logs -f`를 사용하세요.

초기 동기화는 장비에 따라 6시간에서 48시간이 걸리며, 빠른 스토리지 장치는 시간이 덜 걸리고 더 효율적입니다. 후속 재시작은 그렇게 오래 걸리지 않습니다.

### cli_wallet 실행

아래 명령은 실행 중인 컨테이너 내부에서 cli_wallet을 실행하면서 호스트에서 생성한 디렉터리에 `wallet.json`을 매핑합니다.

```
docker exec -it steemd-exchange /usr/local/steemd-default/bin/cli_wallet -w /var/steemwallet/wallet.json
```
