# 소개

이 문서에서는 큐레이션 곡선을 위해 Steem이 사용하는 근사 정수 제곱근 함수를 도출합니다
[여기](https://github.com/steemit/steem/issues/1052).

# MSB 함수

함수 `msb(x)`를 `x >= 1`에 대해 다음과 같이 정의합니다:

- (정의 1a) `msb(x)`는 `x`의 이진 표현에서 최상위 1 비트의 인덱스입니다

다음 정의는 정의 (1a)와 동등합니다:

- (정의 1b) `msb(x)`는 비트 문자열로서 `x`의 이진 표현 길이에서 1을 뺀 것입니다
- (정의 1c) `msb(x)`는 `2 ^ msb(x) <= x`를 만족하는 최대 정수입니다
- (정의 1d) `msb(x) = floor(log_2(x))`

많은 CPU(Intel 386 이후의 Intel CPU 포함)는 하드웨어에서 직접 구현된 특별한 명령으로 머신 워드 크기 정수에 대해 `msb()` 함수를 매우 빠르게 계산할 수 있습니다. C++에서 Boost 라이브러리는 `boost::multiprecision::detail::find_msb(x)`를 사용하여 이 기능에 대한 합리적으로 컴파일러 독립적이고 하드웨어 독립적인 액세스를 제공합니다.

# 근사 로그

정의 1d에 따르면 `msb(x)`는 이미 (다소 조잡한) 근사 base-2 로그입니다. 최상위 비트 아래의 비트는 선형 보간의 소수 부분을 제공합니다. 소수 부분을 *가수(mantissa)*라고 하고 정수 부분을 *지수(exponent)*라고 합니다; 효과적으로 우리는 부동소수점 표현을 재발명했습니다.

다음은 이러한 근사 로그로/로부터 변환하는 몇 가지 Python 함수입니다:

```python
def to_log(x, wordsize=32, ebits=5):
    if x <= 1:
        return x
    # mantissa_bits, mantissa_mask는 x와 독립적
    mantissa_bits = wordsize - ebits
    mantissa_mask = (1 << mantissa_bits)-1

    msb = x.bit_length() - 1
    mantissa_shift = mantissa_bits - msb
    y = (msb << mantissa_bits) | ((x << mantissa_shift) & mantissa_mask)
    return y

def from_log(y, wordsize=32, ebits=5):
    if y <= 1:
        return y
    # mantissa_bits, leading_1, mantissa_mask는 x와 독립적
    mantissa_bits = wordsize - ebits
    leading_1 = 1 << mantissa_bits
    mantissa_mask = leading_1 - 1

    msb = y >> mantissa_bits
    mantissa_shift = mantissa_bits - msb
    y = (leading_1 | (y & mantissa_mask)) >> mantissa_shift
    return y
```

# 근사 제곱근

근사 제곱근 알고리즘을 구성하려면 항등식 `log(sqrt(x)) = log(x) / 2`에서 시작합니다.
`sqrt(x) ~ from_log(to_log(x) >> 1)`을 쉽게 얻을 수 있습니다. 내부 함수 호출을 수동으로 인라인하여 진행할 수 있습니다:

```python
def approx_sqrt_v0(x, wordsize=32, ebits=5):
    if x <= 1:
        return x
    # mantissa_bits, leading_1, mantissa_mask는 x와 독립적
    mantissa_bits = wordsize - ebits
    leading_1 = 1 << mantissa_bits
    mantissa_mask = leading_1 - 1

    msb_x = x.bit_length() - 1
    mantissa_shift_x = mantissa_bits - msb_x
    to_log_x = (msb_x << mantissa_bits) | ((x << mantissa_shift_x) & mantissa_mask)

    z = to_log_x >> 1

    msb_z = z >> mantissa_bits
    mantissa_shift_z = mantissa_bits - msb_z
    result = (leading_1 | (z & mantissa_mask)) >> mantissa_shift_z
    return result
```

# 최적화된 근사 제곱근

먼저 다음 단순화를 고려하십시오:

- 여기서 `msb_z`로 표시된 `z`의 지수 부분은 단순히 `msb_x >> 1`입니다
- `z`의 가수 부분의 MSB는 `msb_x`의 하위 비트입니다
- `z`의 가수 부분의 하위 비트는 단순히 `x`의 가수 부분의 비트가 한 번 시프트된 것입니다

위의 단순화는 더 근본적인 개선을 가능하게 합니다: `x`와 `msb_x`에서 직접 `z`의 가수와 지수를 계산할 수 있습니다. 따라서 중간 결과를 `to_log_x`에 패킹한 다음 즉시 언패킹하는 것은 효과적으로 no-op이 되어 생략할 수 있습니다. 이것은 `wordsize`와 `ebits` 변수가 빠지게 만듭니다. 이러한 매개변수에 대한 선택을 하고 지수 비트를 위해 워드 상단에 추가 공간을 할당하는 것이 완전히 불필요해집니다!

한 가지 미묘한 점은 두 시프트 연산자가 가수 비트의 순 시프트를 초래한다는 것입니다.
`mantissa_bits - msb_x`만큼 왼쪽으로 시프트된 다음 `mantissa_bits - msb_z`만큼 오른쪽으로 시프트됩니다. 따라서 순 시프트는 `msb_x - msb_z`의 오른쪽 시프트입니다.

최종 코드는 다음과 같습니다:

```python
def approx_sqrt_v1(x):
    if x <= 1:
        return x
    # mantissa_bits, leading_1, mantissa_mask는 x와 독립적
    msb_x = x.bit_length() - 1
    msb_z = msb_x >> 1
    msb_x_bit = 1 << msb_x
    msb_z_bit = 1 << msb_z
    mantissa_mask = msb_x_bit-1

    mantissa_x = x & mantissa_mask
    if (msb_x & 1) != 0:
        mantissa_z_hi = msb_z_bit
    else:
        mantissa_z_hi = 0
    mantissa_z_lo = mantissa_x >> (msb_x - msb_z)
    mantissa_z = (mantissa_z_hi | mantissa_z_lo) >> 1
    result = msb_z_bit | mantissa_z
    return result
```
