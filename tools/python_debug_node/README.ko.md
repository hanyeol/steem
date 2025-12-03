# Python Debug Node

## 개요

Python Debug Node는 실행 중인 Steem Debug Node의 생성 및 유지 관리를 자동화하는 래퍼 클래스입니다. Debug Node는 Steem의 플러그인으로, 로컬에 저장된 블록체인을 손상시키거나 라이브 체인에 변경 사항을 전파하지 않고 실제 세계 동작을 모방하는 방식으로 체인 상태를 실시간으로 로컬 수정할 수 있습니다.

debug node에 대한 자세한 정보는 [debug_node_plugin.md](debug-node-plugin.md)에서 찾을 수 있습니다.

## 왜 이것을 사용해야 하는가?

Debug Node Plugin은 매우 강력하지만, 다양한 구성에서 노드를 실행하는 방법과 RPC 또는 WebSocket 인터페이스를 통해 노드와 인터페이스하는 방법에 대한 깊은 지식이 필요합니다. 이 둘 다 개발자 수준의 기술로 간주됩니다. Python은 많은 아마추어 및 숙련된 프로그래머가 사용하는 고급 언어입니다. 라이브 노드와의 인터페이스를 더 쉽게 만들기 위한 Python 라이브러리의 커뮤니티 개발이 이미 있었습니다. 이 플러그인은 노드와 인터페이스하는 것 외에도 Python에서 프로그래밍 방식으로 노드를 시작할 수 있도록 하여 격차를 좁힙니다. 이 모듈은 커뮤니티 멤버 Xeroc의 [Python Steem library](https://github.com/xeroc/python-steemlib)를 활용합니다.

## 어떻게 사용하는가?

먼저 모듈을 설치해야 합니다. `tests/external_testing_scripts`로 이동하여 실행하십시오:

```bash
python3 setup.py install
```

스크립트를 사용하려면 다음을 포함하십시오:

```python
from steemdebugnode import DebugNode
```

### 예제 스크립트

이미 만들어진 몇 가지 예제가 있으며 직접 수정해 볼 수 있습니다.

#### debug_hardforks.py

[debug_hardforks.py](https://github.com/hanyeol/steem/blob/main/tools/python_debug_node/tests/debug_hardforks.py) - 이 스크립트는 debug node를 시작하고, 블록을 재생하고, 하드포크를 예약하고, 마지막으로 하드포크 후 새 블록을 생성합니다. 스크립트는 또한 Xeroc의 라이브러리에서 범용 RPC 인터페이스를 통해 통신하여 결과에 대한 간단한 분석을 수행합니다. 이 경우 블록 생산자의 히스토그램을 생성하여 증인 스케줄링 알고리즘이 제대로 작동하는지 확인합니다. 스크립트의 목적은 주어진 하드포크에 체인 전체를 충돌시킬 수 있는 버그가 없는지 확인하는 것입니다.

#### debugnode.py

[debugnode.py](https://github.com/hanyeol/steem/blob/main/tools/python_debug_node/steemdebugnode/debugnode.py#L212) - 이 스크립트는 훨씬 더 간단합니다. 동일한 파싱 로직이 있지만 테스트 로직이 훨씬 적습니다. 블록체인을 재생하고 주기적으로 상태 업데이트를 출력하여 사용자가 여전히 작동 중임을 알 수 있도록 합니다. 그런 다음 스크립트는 사용자가 RPC 호출 또는 CLI Wallet을 통해 체인과 상호 작용할 수 있도록 대기합니다.

### 기본 사용 패턴

이러한 스크립트의 중요한 부분은 다음과 같습니다:

```python
debug_node = DebugNode(str(steemd), str(data_dir))
with debug_node:
   # 블록체인으로 작업 수행
```

모듈은 `with` 구조를 사용하도록 설정되어 있습니다. 객체 생성은 단순히 올바른 디렉토리를 확인하고 내부 상태를 설정합니다. `with debug_node:`는 노드를 연결하고 내부 RPC 연결을 설정합니다. 그런 다음 스크립트는 원하는 모든 작업을 수행할 수 있습니다. `with` 블록이 끝나면 노드가 자동으로 종료되고 정리됩니다.

노드는 실행 중인 노드의 작업 데이터 디렉토리로 표준 Python `TemporaryDirectory`를 통해 시스템 표준 temp 디렉토리를 사용합니다. 스크립트가 수행해야 하는 유일한 작업은 steemd 바이너리 위치와 채워진 데이터 디렉토리를 지정하는 것입니다. 대부분의 구성에서 이것은 Steem의 git 루트 디렉토리에서 각각 `programs/steemd/steemd` 및 `witness_node_data_dir`입니다.

## TODO / 장기 목표

이것은 Python 테스트 프레임워크의 좋은 시작이지만, 이 스크립트가 부족한 많은 영역이 있습니다:

- **유연성:** 디버깅을 위한 stdout 및 stderr 파이핑에서 실행할 플러그인 및 API를 동적으로 지정하는 것까지 노드에 많은 유연성이 있지만 인터페이스 방법이 부족합니다.
- **API 래퍼:** 이상적으로는 모든 RPC API 호출에 대해 Python 동등 래퍼가 있을 것입니다. 이것은 지루하고 플러그인을 사용하면 해당 API 호출이 개정판마다 변경될 수 있습니다.
- **코드 생성:** 대부분 노출된 Debug Node 호출은 RPC 호출을 위한 래퍼입니다. 대부분 또는 모든 RPC API 호출은 C++ 소스에서 프로그래밍 방식으로 생성될 수 있습니다.
- **테스트 프레임워크:** debug node를 시작한 다음 공통 시작 체인 상태에서 일련의 테스트 케이스를 실행하는 데 사용할 수 있는 간단한 테스트 프레임워크를 도입하는 것도 좋은 진전이 될 것입니다. 이것은 Steem에 절실히 필요한 통합 테스트의 대부분을 해결할 것입니다.
