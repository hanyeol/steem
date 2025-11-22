# 댓글 권한 제어 기능

## 개요

이 문서는 포스트 작성자가 자신의 게시글에 누가 댓글을 달 수 있는지 제어할 수 있는 기능의 구현에 대해 설명합니다. 작성자는 댓글을 달 수 있는 계정의 화이트리스트를 지정하거나, 제한 없이 열어둘 수 있습니다.

## 기능 설명

포스트를 작성할 때, 작성자는 세 가지 댓글 권한 모드 중 하나를 지정할 수 있습니다:

1. **전체 공개 (기본값)** - 누구나 댓글 작성 가능 (기존 동작)
2. **특정 계정으로 제한** - 화이트리스트에 등록된 계정만 댓글 작성 가능
3. **댓글 불가** - 아무도 댓글을 달 수 없음 (빈 화이트리스트)

## 구현 설계

### 1. 프로토콜 변경사항

#### 새로운 Operation 필드

[libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../../libraries/protocol/include/steem/protocol/steem_operations.hpp)의 `comment_operation` 구조체에 새로운 선택적 필드 추가:

```cpp
struct comment_operation : public base_operation
{
   // ... 기존 필드들 ...

   optional< flat_set< account_name_type > > allowed_comment_accounts;

   void validate() const;
   // ...
};
```

**필드 의미:**
- `nullopt` (설정되지 않음) - 모든 사람에게 댓글 허용 (기본값, 하위 호환성 유지)
- 빈 집합 `{}` - 댓글 불가
- 계정이 있는 집합 `{"alice", "bob"}` - 해당 계정들만 댓글 작성 가능

#### 유효성 검사 규칙

`validate()` 메서드에서:

```cpp
void comment_operation::validate() const
{
   // ... 기존 검증 로직 ...

   if( allowed_comment_accounts.valid() )
   {
      // 중복 계정 확인 (flat_set이 이를 방지해야 함)
      // 각 계정 이름 형식 검증
      for( const auto& account : *allowed_comment_accounts )
      {
         FC_ASSERT( is_valid_account_name( account ),
                    "allowed_comment_accounts에 잘못된 계정 이름: ${a}",
                    ("a", account) );
      }

      // 선택사항: 남용 방지를 위한 화이트리스트 최대 크기 설정
      FC_ASSERT( allowed_comment_accounts->size() <= 1000,
                 "댓글 화이트리스트는 1000개 계정을 초과할 수 없습니다" );
   }
}
```

### 2. 체인 상태 변경사항

#### 데이터베이스 객체

[libraries/chain/include/steem/chain/comment_object.hpp](../../../../libraries/chain/include/steem/chain/comment_object.hpp)의 `comment_object`에 새로운 필드 추가:

```cpp
class comment_object : public object< comment_object_type, comment_object >
{
   // ... 기존 필드들 ...

   // 댓글 권한 제어
   // 설정되지 않음 (nullopt)인 경우, 모든 계정이 댓글 작성 가능
   // 계정과 함께 설정된 경우, 해당 계정들만 댓글 작성 가능
   // 설정되었지만 비어있는 경우 (빈 vector), 아무도 댓글 작성 불가
   optional< shared_vector< account_name_type > > allowed_comment_accounts;

   // ... 클래스의 나머지 부분 ...
};
```

**참고:** chainbase는 메모리 매핑된 스토리지를 위해 allocator를 인식하는 컨테이너가 필요하므로, `flat_set` 대신 `shared_vector`를 사용합니다.

#### 인덱스 업데이트

새 필드를 포함하도록 `FC_REFLECT` 매크로 업데이트:

```cpp
FC_REFLECT( steem::chain::comment_object,
            // ... 기존 필드들 ...
            (allowed_comment_accounts)
          )
```

### 3. Evaluator 변경사항

#### Comment Evaluator

[libraries/chain/steem_evaluator.cpp](../../../../libraries/chain/steem_evaluator.cpp)의 `comment_evaluator` 수정:

```cpp
void comment_evaluator::do_apply( const comment_operation& o )
{
   // ... 기존 코드 ...

   // 부모 댓글(답글) 권한 확인 처리
   if( o.parent_author != STEEM_ROOT_POST_PARENT_ACCOUNT )
   {
      const auto& parent = _db.get_comment( o.parent_author, o.parent_permlink );

      // 이 계정이 댓글을 달 수 있는지 확인
      if( parent.allowed_comment_accounts.valid() )
      {
         const auto& allowed_accounts = *parent.allowed_comment_accounts;

         if( allowed_accounts.empty() )
         {
            // 빈 화이트리스트는 댓글 불가를 의미
            FC_ASSERT( false,
                      "이 포스트는 댓글이 비활성화되어 있습니다" );
         }
         else
         {
            // 댓글 작성자가 화이트리스트에 있는지 확인
            auto it = std::find( allowed_accounts.begin(),
                                allowed_accounts.end(),
                                o.author );
            FC_ASSERT( it != allowed_accounts.end(),
                      "계정 ${a}은(는) 이 포스트에 댓글을 달 수 없습니다",
                      ("a", o.author) );
         }
      }
   }

   // ... 댓글 생성/업데이트를 위한 기존 코드 ...

   // 루트 포스트(답글이 아닌 경우), 지정된 경우 댓글 권한 설정
   if( o.parent_author == STEEM_ROOT_POST_PARENT_ACCOUNT )
   {
      if( o.allowed_comment_accounts.valid() )
      {
         _db.modify( comment, [&]( comment_object& c )
         {
            c.allowed_comment_accounts = shared_vector< account_name_type >(
               o.allowed_comment_accounts->begin(),
               o.allowed_comment_accounts->end(),
               c.allowed_comment_accounts.get_allocator()
            );
         });
      }
   }

   // ... 함수의 나머지 부분 ...
}
```

### 4. API 변경사항

#### Database API

[libraries/plugins/apis/database_api/database_api.cpp](../../../../libraries/plugins/apis/database_api/database_api.cpp)의 `database_api`에 댓글 권한 조회 메서드 추가:

```cpp
struct get_comment_permissions_return
{
   bool comments_enabled = true;
   optional< vector< account_name_type > > allowed_accounts;
};

get_comment_permissions_return database_api::get_comment_permissions(
   string author,
   string permlink ) const
{
   return my->_db.with_read_lock( [&]()
   {
      get_comment_permissions_return result;

      const auto& comment = my->_db.get_comment( author, permlink );

      if( comment.allowed_comment_accounts.valid() )
      {
         if( comment.allowed_comment_accounts->empty() )
         {
            result.comments_enabled = false;
         }
         else
         {
            result.allowed_accounts = vector< account_name_type >(
               comment.allowed_comment_accounts->begin(),
               comment.allowed_comment_accounts->end()
            );
         }
      }

      return result;
   });
}
```

## 사용 예제

### 댓글 제한이 있는 포스트 생성

#### 전체 공개 (기본값):

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "내 포스트",
    "body": "포스트 내용",
    "json_metadata": "{}"
  }
}
```

#### 특정 계정만 댓글 작성 가능:

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "내 포스트",
    "body": "포스트 내용",
    "json_metadata": "{}",
    "allowed_comment_accounts": ["bob", "charlie"]
  }
}
```

#### 댓글 불가:

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "내 포스트",
    "body": "포스트 내용",
    "json_metadata": "{}",
    "allowed_comment_accounts": []
  }
}
```

### 댓글 권한 조회

`database_api` 사용:

```javascript
// 포스트에 댓글을 달 수 있는지 확인
const permissions = await api.call('database_api', 'get_comment_permissions', {
  author: 'alice',
  permlink: 'my-post'
});

if (!permissions.comments_enabled) {
  console.log('이 포스트는 댓글이 비활성화되어 있습니다');
} else if (permissions.allowed_accounts) {
  console.log('다음 계정들만 댓글을 달 수 있습니다:', permissions.allowed_accounts);
} else {
  console.log('누구나 댓글을 달 수 있습니다');
}
```

## 테스트

### 단위 테스트

[tests/tests/operation_tests.cpp](../../../../tests/tests/operation_tests.cpp)에 테스트 추가:

```cpp
BOOST_AUTO_TEST_SUITE( comment_permission_tests )

BOOST_AUTO_TEST_CASE( comment_permission_validation )
{ try {
   BOOST_TEST_MESSAGE( "댓글 권한 유효성 검사 테스트" );

   comment_operation op;
   op.parent_author = "";
   op.parent_permlink = "blog";
   op.author = "alice";
   op.permlink = "test-post";
   op.title = "테스트";
   op.body = "본문";

   // 유효: 제한 없음
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // 유효: 빈 화이트리스트 (댓글 불가)
   op.allowed_comment_accounts = flat_set<account_name_type>();
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // 유효: 계정이 있는 화이트리스트
   op.allowed_comment_accounts = flat_set<account_name_type>{"bob", "charlie"};
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // 무효: 잘못된 계정 이름
   op.allowed_comment_accounts = flat_set<account_name_type>{"invalid.name!"};
   REQUIRE_OP_VALIDATION_FAILURE( op, comment_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( comment_permission_enforcement )
{ try {
   BOOST_TEST_MESSAGE( "댓글 권한 시행 테스트" );

   ACTORS( (alice)(bob)(charlie)(dan) )

   generate_block();

   // Alice가 댓글 제한이 있는 포스트 생성
   comment_operation post;
   post.parent_author = "";
   post.parent_permlink = "blog";
   post.author = "alice";
   post.permlink = "restricted-post";
   post.title = "제한된 포스트";
   post.body = "bob과 charlie만 댓글을 달 수 있습니다";
   post.allowed_comment_accounts = flat_set<account_name_type>{"bob", "charlie"};

   push_transaction( post, alice_private_key );
   generate_block();

   // Bob은 댓글을 달 수 있음 (화이트리스트에 있음)
   comment_operation bob_comment;
   bob_comment.parent_author = "alice";
   bob_comment.parent_permlink = "restricted-post";
   bob_comment.author = "bob";
   bob_comment.permlink = "bobs-comment";
   bob_comment.body = "Bob의 댓글";

   push_transaction( bob_comment, bob_private_key );
   generate_block();

   // Dan은 댓글을 달 수 없음 (화이트리스트에 없음)
   comment_operation dan_comment;
   dan_comment.parent_author = "alice";
   dan_comment.parent_permlink = "restricted-post";
   dan_comment.author = "dan";
   dan_comment.permlink = "dans-comment";
   dan_comment.body = "Dan의 댓글";

   REQUIRE_THROW( push_transaction( dan_comment, dan_private_key ), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( no_comments_allowed )
{ try {
   BOOST_TEST_MESSAGE( "댓글 불가 테스트" );

   ACTORS( (alice)(bob) )

   generate_block();

   // Alice가 댓글 불가 포스트 생성
   comment_operation post;
   post.parent_author = "";
   post.parent_permlink = "blog";
   post.author = "alice";
   post.permlink = "no-comments";
   post.title = "댓글 불가";
   post.body = "댓글 비활성화됨";
   post.allowed_comment_accounts = flat_set<account_name_type>(); // 빈 집합 = 댓글 불가

   push_transaction( post, alice_private_key );
   generate_block();

   // Bob은 댓글을 달 수 없음
   comment_operation bob_comment;
   bob_comment.parent_author = "alice";
   bob_comment.parent_permlink = "no-comments";
   bob_comment.author = "bob";
   bob_comment.permlink = "bobs-comment";
   bob_comment.body = "Bob의 댓글";

   REQUIRE_THROW( push_transaction( bob_comment, bob_private_key ), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
```

### 통합 테스트

기능을 종단 간 테스트:

1. 다양한 권한 설정으로 포스트 생성
2. 화이트리스트에 있는 계정과 없는 계정에서 댓글 시도
3. API를 통해 댓글 권한 조회
4. 오류 메시지가 명확하고 도움이 되는지 확인

## 보안 고려사항

1. **화이트리스트 크기 제한** - 남용 방지를 위해 화이트리스트 크기 제한 (권장: 1000개 계정)
2. **가스/리소스 비용** - 큰 화이트리스트가 있는 댓글 operation은 더 많은 계산 리소스가 필요할 수 있음
3. **하위 호환성** - `optional` 필드는 기존 operation이 유효하게 유지되도록 보장
4. **작성자 제어** - 포스트 작성자만 포스트 생성 중에 댓글 권한을 설정할 수 있음
5. **불변성** - 포스트 생성 후 댓글 권한 변경 불가 (편집 시 화이트리스트 수정 안 됨)

## 성능 고려사항

1. **메모리 사용량** - 화이트리스트가 있는 각 댓글은 계정 수에 비례하는 추가 메모리 사용
2. **조회 성능** - 현재 구현에서 화이트리스트 멤버십 확인은 O(n); 큰 화이트리스트의 경우 최적화 고려
3. **직렬화** - operation의 추가 데이터로 트랜잭션 크기 증가

## 향후 개선 사항

향후 하드포크를 위한 잠재적 개선사항:

1. **권한 업데이트** - 작성자가 생성 후 댓글 권한을 수정할 수 있도록 허용
2. **역할 기반 권한** - 특정 평판 또는 스테이크 레벨을 가진 계정의 댓글 허용
3. **블랙리스트 모드** - 화이트리스트 대신 특정 계정 차단
4. **시간 기반 제한** - 특정 시간 이후 댓글 활성화/비활성화
5. **권한 제어 위임** - 작성자가 다른 계정에 권한 관리 위임 허용

## 관련 문서

- [Operation 생성 가이드](../create-operation.md)
- [플러그인 개발 가이드](../plugin.md)
- [테스트 가이드](../testing.md)
- [하드포크 절차](../../../operations/hardfork-procedure.md)

## 참고자료

- **프로토콜 Operations**: [libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
- **체인 Objects**: [libraries/chain/include/steem/chain/comment_object.hpp](../../../../libraries/chain/include/steem/chain/comment_object.hpp)
- **Evaluators**: [libraries/chain/steem_evaluator.cpp](../../../../libraries/chain/steem_evaluator.cpp)
- **Operation 테스트**: [tests/tests/operation_tests.cpp](../../../../tests/tests/operation_tests.cpp)
