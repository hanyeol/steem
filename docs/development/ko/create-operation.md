
이 문서는 STEEM 블록체인에서 완전히 새로운 operation을 생성하기 위한 개발자 문서입니다.

- (1) `smt_operations.hpp`에 `smt_elevate_account_operation` 구조체 정의
- (2) operation struct에 대한 `FC_REFLECT` 정의 생성
- (3) operation struct에 대한 `validate()` 구현
- (4) `steem::protocol::operation`에 operation 추가
- (5) operation에 대한 evaluator 정의
- (6) operation에 대한 필수 권한 정의
- (7) operation에 대한 단위 테스트 정의
- (8) operation에 대한 기존 RPC 객체 수정
- (9) operation을 위한 새 데이터베이스 객체 / 인덱스 생성
- (10) 새 데이터베이스 객체 / 인덱스 사용
- TODO (11) 관련 플러그인에서 operation을 위한 새 RPC / 객체 / 인덱스 정의
- TODO (12) RPC를 사용하여 operation에 대한 `cli_wallet` 기능 구현
- TODO 자율 처리 처리 예제

## Step 1

- (1a) 새로 정의된 operation은 `extensions_type extensions;` 멤버를 포함해야 합니다.
이 정의는 바이너리 직렬화에 예약된 제로 바이트를 추가하여
향후 필드를 추가할 수 있는 "opcode 공간"을 허용합니다.

- (1b) Operation은 `base_operation` 또는 `virtual_operation`을 직접 서브클래스해야 합니다.
Operation은 다른 operation 클래스를 서브클래스하면 안 됩니다. `base_operation`은
사용자가 발행하도록 의도된 operation을 위한 것입니다. `virtual_operation`은
합의 비대상 엔티티이며 사용자가 절대 발행할 수 없습니다; virtual operation은
사용자의 계정 히스토리에 포함하기 위한 자동 작업을 기록합니다 (예:
포스트 보상 받기 또는 시장 주문 체결).

- (1c) Operation 타입은 FC 직렬화와 호환되기 위해 기본 생성자가 있어야 합니다.
대부분의 operation은 기본 생성자만 구현합니다.

## Step 2

- (2a) `FC_REFLECT` 문은 클래스의 정규화된 이름과
필드의 버블 리스트를 포함합니다. `FC_REFLECT`는 다양한 전처리기 기반
타입 내성 작업에 사용되며, 바이너리 및 JSON 형식에 대한 직렬화/역직렬화
코드의 자동 생성을 포함합니다.

- (2b) `FC_REFLECT` 정의에 필드를 넣는 것을 잊어버리면 컴파일은 되지만
버그가 발생합니다 (필드가 직렬화 / 역직렬화되지 않음). 프로덕션에서
이러한 종류의 버그는 잠재적으로 재앙적인 상태 손상을 일으키며 처음에는
항상 눈에 띄지 않습니다. 우리의 CI 서버는 자동화된 스크립트 `check_reflect.py`를 실행하여
코드에서 잊어버린 필드를 스캔하고, 발견되면 빌드를 실패시킵니다.

## Step 3

- (3a) `validate()` 메서드는 `FC_ASSERT` 매크로를 사용하며, 참이 아니면
`fc::exception`을 throw합니다. `validate()`는 operation struct만 사용하여 정확성
조건을 확인할 책임이 있으며, 체인 상태 데이터베이스에 대한 액세스 권한이 없습니다.
체인 상태가 필요한 확인은 `validate()`가 아닌 `do_apply()`에 있어야 합니다.

- (3b) `validate()`에 있어야 하는 확인의 예: 문자열에 불법 문자가 포함되지 않음;
정수 값이 올바른 부호이고 컴파일 타임 하한/상한 내에 있음;
가변 길이 요소가 컴파일 타임 최소/최대 크기 제한 내에 있음; 필드 간 순서 관계
(예: `field_1 >= field_2`); 자산 이름이 컴파일 타임
허용된 자산 이름 집합에 있음.

- (3c) `validate()`에 있을 수 없는 확인의 예: 계정이
존재해야 함 (또는 존재하지 않아야 함); 계정에 충분한 잔액이 있어야 함; 특정
operation 또는 이벤트가 과거에 발생했어야 함 (또는 발생하지 않았어야 함);
특정 날짜 / 블록 번호 / 하드포크 번호가 발생했음 (또는 발생하지 않았음);
`*this` 이외의 operation 내용에 대한 모든 참조. 이러한 모든
확인은 evaluator에 있어야 합니다.

## Step 4

- (4a) `operations.hpp` 파일은 `steem::protocol::operation`
타입을 정의하며, 이는 긴 매개변수 목록을 가진 `fc::static_variant`입니다. (`fc::static_variant`는
[태그된 유니온 타입](https://en.wikipedia.org/wiki/Tagged_union)을 구현하며 C++11
[템플릿 매개변수 팩](http://en.cppreference.com/w/cpp/language/parameter_pack)을 사용하여
잠재적 요소 타입의 목록을 지정합니다). operation이
지정되는 순서가 중요하며, 새 타입을 추가하면 다음 타입들이
다른 정수 ID를 갖게 되어 바이너리 직렬화가 변경되고 하위 호환성이 깨집니다.

- (4b) 새 non-virtual operation은 일반적으로 virtual operation 시작 직전에 추가됩니다.

- (4c) 새 virtual operation은 일반적으로 끝에 추가됩니다.

## Step 5

- (5a) `evaluator.hpp`에 `STEEM_DEFINE_EVALUATOR` 매크로를 추가하여
일부 boilerplate 코드를 생성해야 합니다. 이 매크로는 `evaluator.hpp`에 정의되어 있으며,
생성된 코드의 대부분은 프레임워크에서 요구하는 지원 코드이며
operation 자체에는 영향을 주지 않습니다.

- (5b) operation을 실행하는 실제 코드는 `opname_evaluator::do_apply( const opname_operation& o )`
메서드에 작성됩니다. 이 코드는 `_db` 클래스 멤버의 데이터베이스에 액세스할 수 있습니다.

- (5c) 모든 새 operation과 모든 새 operation 기능은 `libraries/chain/hardfork.d`에 정의된
기능 상수에 대해 `_db.has_hardfork( ... )`를 확인하여 게이트되어야 합니다.

- (5d) 이 예제에서는 `account_object`에 boolean 필드를 추가하고,
나중에 이 필드를 다른 객체에 적용하는 방법을 살펴봅니다.

- (5e) 데이터베이스에서 객체를 가져오는 메서드는 `const` 참조를 반환합니다.
체인 상태를 검색하는 가장 일반적인 방법은
`get< type, index >( index_value )`를 사용하는 것입니다. 예를 들어
[구현](https://github.com/steemit/steem/blob/a0a69b10189d93a9be4da7e7fd9d5358af956e34/libraries/chain/database.cpp#L364)을
참조하십시오.

- (5f) 데이터베이스의 객체를 수정하려면 `db.modify()`를 발행해야 하며,
이는 인수로 `const` 참조와 실제로 필드를 수정하는 콜백
(일반적으로 [lambda](http://en.cppreference.com/w/cpp/language/lambda)로 지정됨)을 받습니다.
(콜백 메커니즘이 사용되는 이유는 `chainbase` 블록체인 데이터베이스 프레임워크가
상태 변이 전후에 부기를 수행해야 하기 때문입니다.)

- (5g) evaluator는 `database::initialize_evaluators()`에 등록되어야 합니다.

## Step 6

- (6a) 필수 권한은
`get_required_active_authorities()`,
`get_required_posting_authorities()`,
`get_required_owner_authorities()`에 의해 구현되며, 호출자가 제공한
`flat_set`을 수정합니다. 이러한 메서드는 일반적으로 인라인입니다.

- (6b) operation의 필드만으로 필수 권한을 계산하기에 충분한 정보를 제공해야 합니다.
이 요구사항의 결과로, 새 operation을 설계할 때 때로는
데이터베이스에서 이미 사용 가능한 정보를 명시적으로 복제하는 필드를 추가해야 합니다.

## Step 7

- (7a) SMT 기능에 대한 단위 테스트는 `smt_tests.cpp`에 있어야 합니다.

- (7b) Operation은 별도의 테스트가 있어야 합니다: `validate` 테스트,
`authorities` 테스트 및 `apply` 테스트.

- (7c) 단위 테스트가 통과하지만 버그 또는 사양 변경이 발견되는 경우,
버그를 수정하는 것 외에도 버그를 보여주는 코드를
단위 테스트에 추가해야 합니다. 이제 단위 테스트는 버그 수정 코드 없이는 실패하고,
버그 수정 코드와 함께 통과해야 합니다.

## Step 8

- (8a) libraries/chain의 객체는 블록체인 코어의 일부이며, 합의에 사용됩니다
- (8b) 블록체인 코어 객체에 필드를 추가할 때, JSON 클라이언트가 사용할 수 있도록
RPC 객체에 동일한 필드를 추가해야 합니다
- (8c) 필드 정의 및 리플렉션 추가
- (8d) RPC 객체 생성자에서 DB 객체 필드에서 RPC 객체 필드 초기화

## Step 9

- (9a) `steem_objects.hpp`의 `enum object_type`에 `smt_token_object_type`를 추가하고
해당 파일 하단의 `FC_REFLECT_ENUM` 버블 리스트에 추가
- (9b) `steem_objects.hpp`에서 `class smt_token_object;`를 선언 (정의하지 않음)
- (9c) `steem_objects.hpp`에서 `typedef oid< smt_token_object > smt_token_id_type` 정의
- (9d) `smt_objects` 디렉토리에 객체 헤더 파일 생성 (객체당 하나의 헤더 파일).
`smt_objects.hpp`에서 새 헤더를 포함합니다.
- (9e) 모든 SMT 객체는 합의이므로 `steem::chain` 네임스페이스에 존재해야 합니다
- (9f) 객체 클래스는 `object< smt_token_object_type, smt_token_object >`를 서브클래스해야 합니다
- (9g) 생성자는 다른 객체 클래스와 동일하게 정의되어야 하며,
`Constructor` 및 `Allocator` 템플릿 매개변수를 사용합니다.
- (9h) 가변 길이 필드는 특별한 주의가 필요합니다. 이러한 필드는
interprocess 호환 타입이어야 하며 생성자에서 전달된 `allocator`가 있어야 합니다.
실제로 이는 문자열이 `shared_string`이어야 하고, 컬렉션(vector, deque)이
`boost::interprocess` 타입 중 하나여야 함을 의미합니다. `transaction_object::packed_trx`,
`comment_object::permlink`, `feed_history_object::price_history` 필드의 예를 참조하십시오.
- (9i) 첫 번째 필드는 `id_type id;`이어야 합니다
- (9j) 대부분의 필드는 기본 초기화되거나 0, 비어 있거나 컴파일 타임
기본값으로 설정되어야 합니다. 일반적으로 클래스 생성자에서 수행되는 유일한 필드 초기화는
allocator 전달, 정수 필드를 0으로 설정, 호출자가 제공한
`Constructor` 콜백 실행입니다. "실제" 초기화는 외부 정보 (데이터베이스 및 operation에서)에
액세스할 수 있는 해당 콜백에서 수행됩니다.
- (9k) 모든 필드는 버블 리스트의 `FC_REFLECT`에 전달되어야 합니다
- (9l) 기본 `by_id` 이외의 인덱스에 대한 `struct` 정의는 클래스 뒤에 있어야 합니다;
`by_id`는 객체 클래스에 의해 *절대* 정의되어서는 안 됩니다.
- (9m) 인덱스 타입이 정의됩니다. `by_id` 인덱스는 항상 정의되어야 합니다. 자세한 내용은 아래 참고 사항 및
Boost `multi_index_container` 문서를 참조하십시오.
- (9n) `CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )`
매크로를 호출해야 합니다. 전역 범위(모든 네임스페이스 외부)에서 호출해야 합니다.
- (9o) `database::initialize_indexes()`에서 `add_core_index< smt_token_index >(*this);`를 호출하여
데이터베이스에 객체 타입을 등록합니다.

### Step 9 추가 설명

Step 9는 약간의 설명이 필요합니다.

- (9a) 각 객체 타입에는 해당 객체 타입을 나타내는 정수 ID가 있습니다. 이러한 타입 ID는
`steem_object_types.hpp`의 `enum`으로 정의되며, 새 객체는 여기에 추가되어야 합니다. SQL
용어로, 각 *데이터베이스 테이블*에 정수 ID가 있다고 상상하면, `smt_token_object_type`은
`smt_token_object` *테이블*을 참조하는 ID 값입니다.

- (9c) 객체 ID는 모두 정수이지만, 컴파일 타임에 각 ID 타입 변수에
ID가 참조하는 테이블을 나타내는 "노트"가 첨부됩니다. 이는
클래스 이름을 템플릿 매개변수로 사용하는 `chainbase::oid` 클래스에 의해 구현됩니다.
일반적인 코드에서 필요한 템플릿 호출 횟수를 줄이기 위해 (그리고 `chainbase` 또는 그 전신의
이전 버전으로 먼저 개발된 코드의 이식을 용이하게 하기 위해), 타입 별칭
`typedef oid< smt_token_object > smt_id_type`이 `steem_object_types.hpp`에 추가됩니다.

- (9f) `smt_token_object` 클래스는
`chainbase::object< smt_token_object_type, smt_token_object >`를 서브클래스합니다. 이것은
[curiously recurring template pattern](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)으로,
템플릿과 다형성의 상호 작용을 허용하는 타입 매개변수의 라우팅을 포함합니다. 복잡하게 들리지만,
`chainbase.hpp`의 `object` 클래스 정의는 단순히
`typedef oid< smt_token_object > id_type;`을 수행하는 데 사용된다는 것을 명확히 합니다.

위의 결과로, `smt_token_id_type`, `oid< smt_token_object >` 및
`smt_token_object::id_type`은 모두 정수 객체 ID를 나타내는 타입을 참조하며,
컴파일 타임 "노트"가 첨부되어 이 객체 ID가 `smt_token_object`
데이터베이스 테이블의 기본 키임을 나타냅니다. 해당 데이터베이스 테이블의 테이블 ID는
`smt_token_object_type` 또는 `smt_token_object::type_id`로 주어진 정수 값입니다.

- (9h) 데이터베이스에 메모리 매핑된 파일을 사용하고 있으며, 데이터베이스가 메모리를 할당할 때
메모리 매핑된 파일로 이동해야 합니다 (힙이 아님). 데이터베이스 객체의 가변 길이 필드는
allocator가 전달되어야 하며 멀티프로세스 액세스와 호환되어야 합니다.
실제로 이는 `boost::interprocess::vector` 및 `shared_string`을 사용하는 것을 의미합니다.
`transaction_object::packed_trx`, `comment_object::permlink`,
`feed_history_object::price_history` 필드의 예를 참조할 수 있습니다.

- (9i) 각 개별 객체에는 해당 객체의 "기본 키" 역할을 하는 `id`라는 정수 객체 ID 필드가 있습니다.
이는 모든 객체에 필요하며, `chainbase` 데이터베이스 프레임워크가 실행 취소 기능을 구현하는 데 사용됩니다.
생성된 각 객체에는 다음 순차적으로 사용 가능한 객체 ID가 할당됩니다.

- (9l) `chainbase`의 일부 기능에는 `by_id` 필드가 필요합니다. `chainbase`는
Steem에 긴밀하게 결합되지 않은 재사용 가능한 라이브러리로 설계되었기 때문에
`steem` 네임스페이스에 대한 참조가 포함되지 않습니다. 따라서 `by_id` 이름은
`chainbase::by_id`를 참조해야 합니다. `steem::chain` 네임스페이스에서 `struct by_id;`를 정의하면
나중에 컴파일 단위에서 정의된 모든 인덱스가 한정자 없이 `by_id`를 참조하면
잘못되거나 모호한 타입 참조가 됩니다. 결과는
올바르게 작동하지 않을 가능성이 높으며 컴파일되지 않을 수도 있습니다.

- (9m) `smt_token_index`의 템플릿 정의는 위협적으로 보입니다 (그리고 `control_account`와 같은
인덱스의 정의는 더욱 그렇습니다). 인덱스는
Boost 라이브러리에서 제공하는 `multi_index_container` 타입에 의해 사용됩니다. `multi_index_container` 라이브러리는
여러 "키"가 잠재적 랜덤 액세스/반복을 위해 정의될 수 있고 "복합 키"
(여러 필드로 구성)도 지원하는 인메모리 SQL 데이터베이스의 기능을 대략 구현합니다.
모든 "데이터베이스 테이블"에 대한 모든 타입 정보가 컴파일 타임에 알려져 있으므로,
`multi_index_container`는 빠르고 효율적입니다.
따라서 `smt_token_index` 정의는 기본적으로 사용 가능한 인덱스를 설명하는 DDL입니다 --
더 복잡한 구문을 가진 SQL `CREATE INDEX` 문과 같습니다. 대부분의 경우,
인덱스 정의는 기존의 작동하는 정의에서 복사할 수 있는 "보일러플레이트"로 처리할 수 있습니다.
구문에 대한 자세한 정보는 Boost 문서에서 확인할 수 있습니다.

- (9m) `by_id` 인덱스는 `chainbase` 인프라가 실행 취소 기능을 구현하는 데 사용됩니다.

- (9m) Steem에서 사용되는 모든 인덱스는 `ordered_unique`이어야 합니다. 이론적으로 해시되거나 비고유
인덱스가 일부 상황에서 허용될 수 있으며 성능 이점을 제공할 수 있습니다.
그러나 과거 경험에 따르면 이러한 인덱스의 정의되지 않은 반복 순서는
상태 손상 버그의 잠재적 원인입니다 (실제로 이러한 인덱스의 반복 순서는
노드가 시작되는 특정 시점과 블록 및 트랜잭션이 수신되는 특정 순서에 따라 달라집니다).

## Step 10

- (10a) evaluator 또는 블록당 처리 중에 `database::create()`, `database::modify()` 및 `database::remove()`를
사용할 수 있습니다 (이러한 메서드는 실제로 `chainbase`에서 구현되며, `database` 클래스가 파생됩니다).
- (10b) `create()` 및 `modify()` 함수는 객체의 새 필드 값을 채워야 하는 콜백을 받습니다.
- (10c) `modify()` 및 `remove()` 함수는 기존 객체에 대한 참조를 받습니다.
- (10d) 객체에 대한 참조를 얻으려면 `db.get()`을 사용하여 인덱스 값으로 객체를 조회할 수 있습니다.
예를 들어 `db.get< smt_token_object, by_control_account >( "alice" );`는 객체가 존재하지 않으면 예외를 throw합니다.
evaluator에서 이러한 예외는 operation
(및 이를 포함하는 트랜잭션 또는 블록)을 실패시킵니다.
- (10e) 잠재적으로 존재하지 않는 객체를 얻으려면 `db.find()`를 사용할 수 있으며,
이는 `db.get()`과 같지만 포인터를 반환하거나 객체가 존재하지 않으면 `nullptr`을 반환합니다.
- (10f) 일부 인덱스의 순서대로 객체를 반복하려면
`auto& idx = get_index< smt_token_index >().indices().get< by_control_account >()`를 호출하여
`multi_index_container` 인덱스에 대한 참조를 얻은 다음 `begin()`, `end()`를 사용하여
끝에서 순회하거나 `lower_bound()`, `upper_bound()`를 사용하여 임의의 지점에서 순회를 시작할 수 있습니다.
많은 예가 코드 및 `multi_index_container` 문서에 있습니다.
동시 수정에 주의하십시오!
- (10g) 복합 키를 `db.get()`, `db.find()` 또는 위의 인덱스
반복자 함수에 전달하려면 `boost::make_tuple()`을 사용합니다.
- (10h) 객체가 있고 해당 위치에서 반복을 시작하려면
`idx.iterator_to()`를 사용합니다. 이 기술은 페이지네이션된 쿼리를 구현하는 RPC 메서드에서 사용됩니다.
- (10i) 객체 조회 메서드는 일반적으로 `const` 참조/포인터를 반환합니다.
`multi_index_container`의 반복 트리 구조 및 `chainbase`의 실행 취소 기능은
모든 수정에 대해 일부 "pre-" 및 "post-" 부기를 구현합니다. 조회에
`const` 참조를 사용하면 컴파일러가 적절한 부기를 수행하는
`db.create()`, `db.modify()` 및 `db.remove()` 메서드를 통해서만 수정이 발생하도록 강제할 수 있습니다.
객체 수정을 허용하는 non-`const` 참조는
수정이 발생하는 콜백 본문 내에만 존재합니다.

### Step 10 추가 설명

- (10) 다음 논의는 `smt_token_object` 예제 코드에 특정하며
다른 객체에는 직접 적용되지 않습니다.

`smt_token_index`에 대해 두 개의 단일 열 인덱스가 정의됩니다, `id` 필드의 `by_id`와
`control_account` 필드의 `by_control_account`.

이제 각 SMT에 대해 객체가 생성되므로, 객체에 연결된 SMT가 있는지
설명하는 데이터베이스의 `is_smt` 필드는 중복 정보이며 제거할 수 있습니다. JSON-RPC 클라이언트에
동일한 인터페이스를 계속 제공하려고 하므로 RPC 객체에 `is_smt` 필드를 유지합니다.
그러나 이제 일치하는 `control_account`를 가진 `smt_token_object`가 존재하는지에 따라 채웁니다.

또한 evaluator에서 `is_smt` 확인을 삭제합니다. 그러나 계정이
두 번 승격될 수 없다는 것을 테스트하는 단위 테스트 코드는 여전히 통과합니다. 통과하는 이유는
생성된 객체가 고유 인덱스에 대해 기존 객체와 동일한 키 값을 갖는 경우 객체 생성이 예외를 throw하기 때문입니다.
따라서 두 번째 계정 생성은 `control_account` 필드의 고유성 제약 조건 위반으로 인한 예외로 인해 실패합니다.
