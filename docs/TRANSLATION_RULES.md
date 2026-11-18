# Deterministic Translation Rules

This document defines the contract for mechanically translating Go source into Eo-based C++.

The goal is not merely to produce semantically equivalent C++. The translated result should remain structurally aligned with the Go source so that humans and tooling can compare the implementations and reapply upstream Go changes with minimal semantic interpretation.

For translation principles and compatibility scope, see [Translation Principles](./TRANSLATION.md). Construct-specific documents may define additional canonical mappings, such as [Goroutine Translation Design](./GOROUTINE.md) and [Select Translation Design](./SELECT.md).

## Deterministic output

A supported Go construct should have one canonical Eo translation shape.

When multiple C++ forms would preserve behavior, translators should choose the documented canonical form rather than selecting among stylistic alternatives.

This is especially important for LLM-based translation: the same Go structure should produce substantially the same C++ structure across translation runs.

Translation rules should therefore avoid choices based on taste, local refactoring opportunities, or inferred intent that is not represented in the source.

## Preserve source correspondence

Unless C++ or a documented Eo mapping requires otherwise, preserve:

- file-level declaration order;
- function and method boundaries;
- identifier spelling;
- statement order;
- control-flow shape and nesting;
- declaration placement and scope;
- switch/select clause order;
- field order;
- meaningful source comments.

Do not perform unrelated refactoring during mechanical translation.

In particular, do not introduce helper extraction, condition inversion, loop restructuring, early-return rewrites, declaration hoisting, variable merging, expression simplification, or other idiomatic C++ transformations merely because they appear cleaner.

Native C++ redesign and optimization belong in a separate step after a mechanically translated baseline exists.

## Preserve evaluation boundaries

Go evaluation order and evaluation timing are part of the translation contract whenever they are observable.

Do not move an expression across a statement or control-flow boundary unless the corresponding translation rule explicitly permits it.

When C++ requires a temporary to preserve evaluation order, ownership, lifetime, or coroutine behavior, introduce the smallest local temporary needed and keep it adjacent to the source construct it represents.

Do not introduce temporaries solely for stylistic reasons.

## Identifier mapping

Preserve Go identifier spelling whenever it is a valid, non-reserved C++ identifier.

If an identifier must change because of C++ syntax, a reserved identifier, a semantic conflict, or an explicit Eo mapping, use the documented mapping rather than inventing a local rename.

A translator must not rename identifiers merely to follow a C++ naming convention.

### Redeclared names

Go lets a local variable hide anything of the same name from an enclosing block or the package: a variable, a constant, a function, or a type. C++ allows the same hiding, with two differences, so a local variable declaration of `x` that hides another `x` is renamed in two cases:

1. The declaration refers to the hidden `x`, at any block depth, as in `x := x`, `x, y := x+1, 0`, `g := g()` for a function `g`, or `point := point{1, 2}` for a type `point`. In C++ the scope of a variable starts at its declarator, so the `x` in its own initializer names the new variable, and `auto x = x;` and `auto g = g();` are ill-formed.
2. The declaration is directly in the body of a statement whose own clause declares `x`. C++ rejects such a declaration. This covers:
   - the body of a three-clause `for` or a `range` loop whose clause declares `x`;
   - the `if` and `else` branches of an `if` whose init statement declares `x`;
   - a case of a `switch` whose init statement declares `x`, if the translated case body is not a block of its own.

The renamed variable is `_eo_shadow_<x>`. If what it hides is itself `_eo_shadow_<x>`, it becomes `_eo_shadow_<x>_2`, and each further level counts up by one. Every use of the variable in its scope uses the new name. The initializer and other uses of the hidden name keep its own C++ name:

```go
x := 1
if x > 0 {
	x := x + 1
	{
		x := x * 10
		fmt.Println(x)
	}
}
```

```cpp
auto x = int64_t{1};
if (x > 0) {
  auto _eo_shadow_x = x + 1;
  {
    auto _eo_shadow_x_2 = _eo_shadow_x * 10;
    fmt::Println(_eo_shadow_x_2);
  }
}
```

A renamed variable is boxed by the same rules as any other, as in `auto _eo_shadow_v = New<int64_t>(v);`, and a boxed hidden variable is read through its box, as in `auto _eo_shadow_v = *v;`. Any other hiding declaration keeps its Go name.

### Package-level names hidden by members

Inside a struct definition, and inside the out-of-class definitions of its methods, C++ looks up a name among the struct's members before the package. Go does not: a method sees fields and other methods only through its receiver. C++ also makes a struct definition ill-formed when a name used in it means something else after a member of that name is declared, as in `struct Node { Config Config{}; };`, which GCC rejects. So inside a struct definition and the definitions of its methods, a package-level type, function, variable, or constant whose name is also the name of a member of that struct (a field, an embedded field, or a method) is written with the global qualifier, as `::Config`. This includes the struct's own name when a field has it: `type T struct { T int }` declares `auto operator==(const ::T&) const -> bool = default;`. Names that no member shares stay unqualified:

```go
type Node struct {
	Config Config
	Value  int
}

func (n Node) Hash() Hash {
	return Hash{Lo: n.Value, Hi: n.Config.Depth}
}

func (n *Node) Size() int {
	return Size(n.Value) + n.Config.Depth
}
```

```cpp
struct Node {
  ::Config Config{};
  int64_t Value{};

  auto operator==(const Node&) const -> bool = default;

  static auto Hash(Node n) -> ::Hash;
  static auto Size(ptr<Node> n) -> int64_t;
};

auto Node::Hash(Node n) -> ::Hash {
  return ::Hash{.Lo = n.Value, .Hi = n.Config.Depth};
}

auto Node::Size(ptr<Node> n) -> int64_t {
  return ::Size(n->Value) + n->Config.Depth;
}
```

A type that the translation spells where the Go source does not, such as the type of a boxed variable, is qualified in the same way when a local variable of the same name is in scope, including the variable being declared, whose scope starts at its declarator. The Go source cannot name the type there, but the translation can: `point := 1` followed by a boxed `p := mk(point)` is `auto p = New<::point>(mk(point));`, and a boxed `point := mk()` is `auto point = New<::point>(mk());`.

A member can also hide an Eo name that `using namespace eo;` makes visible, such as a field named `ptr`, `ref`, `slice`, or `New`. Inside such a struct and its method definitions, that Eo name is written `eo::ptr`, `eo::ref`, and so on (see [#83](https://github.com/conr2d/eo/issues/83) for Eo name collisions in general).

`::` reaches only package-level names. A struct type declared inside a function is not one, so a local struct type that a member or a local variable hides where the translation spells it has no translation yet: a field or embedded field named after another local type or after its own struct, and a boxed variable named after its local type.

`::` names the global namespace, where a translated package's declarations are until packages have a mapping of their own.

## Canonical construct mappings

Only mappings that have been explicitly frozen are canonical.

### Basic types

Go's basic types map to fixed-width C++ types. Eo targets 64-bit platforms, where Go's `int` and `uint` have 64 bits, and `<eo/integer.h>`, which `<eo/core.h>` includes, checks that `float` and `double` are IEEE 754 binary32 and binary64.

| Go | C++ |
| --- | --- |
| `bool` | `bool` |
| `int`, `uint` | `int64_t`, `uint64_t` |
| `int8`, `int16`, `int32`, `int64` | `int8_t`, `int16_t`, `int32_t`, `int64_t` |
| `uint8`, `uint16`, `uint32`, `uint64` | `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` |
| `byte`, `rune` | `uint8_t`, `int32_t` |
| `uintptr` | `uintptr_t` |
| `float32`, `float64` | `float`, `double` |
| `string` | `std::string` (see [Strings](#strings)) |

The zero value is written with `{}`: `var n int` is `int64_t n{};`, and `var f float64` is `double f{};`. `int64_t n;` would leave `n` indeterminate. A boxed variable is `New<int64_t>()` (see [Boxed variables](#boxed-variables)).

#### Arithmetic operators

Go's signed `+`, `-`, `*`, and unary `-` wrap on overflow. C++ leaves signed overflow undefined, so Eo requires `-fwrapv`, under which GCC and Clang wrap. The `eo` CMake target passes it as a public compile option to every target that links it. Translated code compiled without `-fwrapv` is outside the mapping. With it, UBSan does not report signed overflow, which is not undefined there.

| Go | C++ |
| --- | --- |
| `x + y`, `x - y`, `x * y`, `-x`, `+x` | the same |
| `x & y`, `x \| y`, `x ^ y` | the same |
| `^x` | `~x` |
| `x &^ y` | `x & ~y` |
| `x / y`, `x % y` of integers | `eo::div(x, y)`, `eo::rem(x, y)`, or `x / y`, `x % y` when `y` is a constant other than `-1` |
| `x / y` of floats | `x / y` |
| `x << n`, `x >> n` | `eo::shl(x, n)`, `eo::shr(x, n)`, or `x << n`, `x >> n` when `n` is a constant less than the width of `x`'s type |
| `x /= y`, `x %= y` of integers, `x <<= n`, `x >>= n` | `eo::div_assign(x, y)`, `eo::rem_assign(x, y)`, `eo::shl_assign(x, n)`, `eo::shr_assign(x, n)`, or the C++ operator under the condition above |
| `x &^= y` | `x &= ~y` |
| the other compound assignments, `x++`, `x--` | the same |
| `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `\|\|`, `!` | the same |

`eo::div` and `eo::rem` panic with `runtime error: integer divide by zero` when `y` is 0. For the most negative value of a signed type divided by `-1`, `eo::div` returns that value, and `eo::rem` returns 0, as Go does; C++ leaves both undefined. Go rejects a constant divisor of 0, and `-1` is never an unsigned divisor, so a constant divisor needs a helper only when it is `-1`. Both round toward zero, and the remainder has the sign of `x`, as in C++.

`eo::shl` and `eo::shr` take a count of any integer type and panic with `runtime error: negative shift amount` when it is negative. A count of at least the width shifts every bit out: `eo::shl` gives 0, and `eo::shr` gives -1 for a negative signed `x` and 0 otherwise. C++ leaves such a shift undefined. `>>` of a signed value is arithmetic in both languages. Go rejects a negative constant count, so a constant count needs a helper only when it is at least the width, which Go allows when `x` is not a constant.

The helpers return the type of `x`. They are function objects, called qualified, with the same collision rule as the string helpers (see [Strings](#strings)). The `_assign` forms take `x` by reference and evaluate it once, as Go does: `s[i] /= d` is `eo::div_assign(eo::at(s, i), d)`. Their arguments follow [Calls next to slice and string operands](#calls-next-to-slice-and-string-operands), like those of any helper.

C++ converts an operand narrower than `int` to `int` before an arithmetic or bitwise operator, so the result of `a + b` for two `int8` values is an `int`, which can be outside `int8`'s range. When the Go type of an operator expression in the table is `int8`, `int16`, `uint8`, or `uint16`, convert its result back: `a + b` is `int8_t(a + b)`, `^b` is `uint8_t(~b)`, and `s[i] - '0'` is `uint8_t(eo::at(s, i) - '0')`. The conversion wraps modulo 2<sup>N</sup>, as Go's arithmetic does. A helper already returns the type of `x`, and a compound assignment, `++`, and `--` convert when they assign, so none of them needs a conversion. A comparison yields `bool` and needs none either.

Go's operator precedence differs from C++'s for the bitwise operators and shifts: Go groups `<<`, `>>`, `&`, and `&^` with `*`, and `|` and `^` with `+`, all above the comparisons, while C++ places the shifts below `+` and `&`, `^`, and `|` below the comparisons. So an operand that is itself a binary operator expression is written in parentheses when either operator is `<<`, `>>`, `&`, `&^`, `|`, or `^`, even where Go has none: `a + b<<c` is `a + (b << c)`, `x&mask == 0` is `(x & mask) == 0`, and `a &^ b + c` is `(a & ~b) + c`. Parentheses in the Go source are kept, except around an expression that is a whole argument of a helper or a conversion, which already delimits it: `(a+b)/2` of `int8` values is `int8_t(int8_t(a + b) / 2)`, and `7/(n+2)` is `eo::div(int64_t{7}, n + 2)`.

#### Conversions

| Go | C++ |
| --- | --- |
| `T(x)` between integer types | `T(x)`, as in `int8_t(x)` and `uint64_t(i)` |
| `T(x)` from an integer type to a float type | `double(x)`, `float(x)` |
| `T(x)` from a float type to an integer type | `T(x)`, as in `int64_t(f)` |
| `float64(x)`, `float32(x)` between float types | `double(x)`, `float(x)` |

A conversion between integer types keeps the low N bits, sign-extending a signed source, as both languages do. A conversion to a float type rounds to the nearest value, ties to even. A conversion from a float type truncates toward zero. When the result is out of the integer type's range, or the value is NaN, Go leaves the result to the implementation and C++ leaves it undefined, so a program that converts such a value is outside the mapping. A `float64` beyond `float32`'s range converts to an infinity in both languages.

A conversion of a constant is a constant, so it is written as described in [Untyped constants](#untyped-constants), not as a conversion. Conversions to and from strings are in [Strings](#strings) and [Slices](#slices).

#### Untyped constants

Go evaluates a constant expression exactly and converts an untyped constant to a type only where it is used. C++ would evaluate the same expression in fixed-width types with C++ precedence: `1<<2 + 1` is 5 in Go and 8 in C++, and `x := 1 << 40` overflows a C++ `int`. So the translation writes a constant from its value, which the Go type checker records (`go/types` `TypeAndValue.Value`), rather than from its source. `T{L}` below is the form in [Untyped constant arguments](#untyped-constant-arguments): `T` is the translated type that the constant converts to, and `L` is a C++ literal of the folded value.

- A variable declared without a type, `x := c` or `var x = c`, is `auto x = T{L};`, where `T` is the type of a typed constant `c`, as in `x := int8(1)`, which is `auto x = int8_t{1};`, or the default type of an untyped one: `int64_t` for an integer constant, `int32_t` for a rune constant, and `double` for a floating-point one. `n := 10` is `auto n = int64_t{10};`, `r := 'a'` is `auto r = int32_t{97};`, and `f := 1.5` is `auto f = double{1.5};`. A boolean constant is `true` or `false`, and a string constant follows [Strings](#strings), so their C++ types are already Go's: `ok := true` is `auto ok = true;`.
- A numeric constant converted to an interface, such as an operand of `fmt.Println`, also takes its default type and is written `T{L}`: `fmt.Println(1, 'a', 2.5)` is `fmt::Println(int64_t{1}, int32_t{97}, double{2.5})`.
- A constant that is the first operand of `eo::div`, `eo::rem`, `eo::shl`, or `eo::shr` is written `T{L}`, because the helper takes its type from that operand: `1 << n` with an `int64` result is `eo::shl(int64_t{1}, n)`.
- Anywhere else, Go converts the constant to the type of the context, such as the other operand of an operator, a parameter, an assigned or declared variable, a result, or a slice element. The constant is written as in the source when it is a single literal, or a single literal after `-`, of one of these kinds:

  - an integer literal whose value is from -2147483647 to 2147483647, so that C++ reads it as an `int`, converted to an integer type or to `float64`: `i < 10`, `x * -2`, `New<int64_t>(0)`;
  - a rune literal whose value is below `0x80`, as an operand of an operator that the translation writes as a C++ operator, where C++ promotes the `char` to `int`: `b == 'a'`, `s[i] - '0'`. A rune literal anywhere else, such as an index or an argument, is written as its decimal value, because the helpers reject a `char`: `table['a']` is `eo::at(table, 97)`, and `x << 'a'` is `eo::shl(x, 97)`;
  - a floating-point literal converted to a float type: `f * 0.5`. A literal whose value rounds to zero in the target type drops a leading `-`, because a Go constant has no negative zero: `f * -0.0` is `f * 0.0`, and `-1e-400` converted to `float64` is `0.0`.

  `_` digit separators are removed, an octal prefix `0o` or `0O` is written `0`, and a floating-point literal converted to `float32` gets an `f` suffix, as in `0.25f`. The usual arithmetic conversions of C++ then give the operation Go's type, and a narrow result is converted back as described in [Arithmetic operators](#arithmetic-operators). Any other constant is written `T{L}`: `x + 1<<20` is `x + int64_t{1048576}`, `u & 0xffffffff` with a `uint32` `u` is `u & uint32_t{4294967295u}`, `n * 2.0` with an `int` `n` is `n * int64_t{2}`, and `[]float32{16777217}` is `slice<float>::of({float{1.6777216e+07f}})`.

A constant that names a declared constant, such as `N` after `const N = 10`, or an expression that contains one, is not covered yet, because constant declarations have no mapping ([#68](https://github.com/conr2d/eo/issues/68)). The argument temporaries of `go` and `defer` statements are always `T{L}`, even for a single literal (see [Untyped constant arguments](#untyped-constant-arguments)).

#### Printing numbers

`fmt.Println` prints every integer type in decimal, including `int8_t` and `uint8_t`, which a C++ stream would print as characters. It prints a `float` or `double` as Go's `%v` does, which differs from fmt's `{}`: the shortest decimal that reads back as the same value, in exponent form when the decimal exponent is below -4 or at least 6, as in `1e+06` and `1.234567e+06`, and `+Inf`, `-Inf`, and `NaN`. Floats inside slices, maps, and structs print the same way.

Not covered yet: `complex64` and `complex128`, constant declarations and named constants ([#68](https://github.com/conr2d/eo/issues/68)), defined types whose underlying type is a basic type, such as `type Celsius float64` ([#104](https://github.com/conr2d/eo/issues/104)), and the `math` package.

### Untyped constant arguments

Go evaluates an untyped constant expression exactly, in its own kind, and only then converts its value to the type the context requires. For a call argument, that type is the parameter type, or, for an interface parameter, the constant's default type (`bool`, `rune`, `int`, `float64`, `complex128`, or `string`), which is then converted to the interface.

This rule applies to the argument temporaries of `go` and `defer` statements, which are always written `T{L}`, even for a single literal. Every other constant follows [Untyped constants](#untyped-constants).

Do not translate the source expression of an untyped constant argument. C++ would evaluate it in a fixed-width type with its own operator precedence. Instead, write the folded constant value that the Go type checker reports for the argument (`go/types` `TypeAndValue.Value`) as `T{L}`, where `T` is the translated target type, including a translated defined type such as `type Celsius float64`, and `L` is a C++ literal of the value:

- integer: `L` is the decimal value, with a `u` suffix when the type is unsigned; the minimum value of a signed type is written `-M - 1`, where `M` is the type's maximum;
- floating-point: the recorded value `v` is already rounded to the type. Let `x` be `v` as a Go float (`constant.Float64Val` or `constant.Float32Val`) and `s` be `strconv.FormatFloat(x, 'g', -1, bits)`. If `constant.Compare(constant.MakeFromLiteral(s, token.FLOAT, 0), token.EQL, v)` holds, the decimal `s` is exactly `v`, and `L` is `s`, with `.0` appended when it contains neither `.` nor `e`. Otherwise `L` is the hexadecimal floating literal `strconv.FormatFloat(x, 'x', -1, bits)`, which is always exact. Add an `f` suffix when the underlying Go type is `float32`;
- boolean: `L` is `true` or `false`;
- string: `L` is the folded value as a C++ string literal, written as described in [String literals](#string-literals), as in `std::string{"a\x{00}b"s}` when `T` is `std::string`.

Untyped complex constant arguments are not frozen, because no translation of complex values is frozen yet. A `go` or `defer` statement with such an argument is provisional.

```go
go f(1 / 3)       // func f(x float64); Go value 0
go f(1.0 / 3)     // func f(x float64)
go f(7 / 2 * 2.0) // func f(x float64); Go value 6
go g(1<<2 + 1)    // func g(n int64); Go value 5
go h(37.5)        // func h(c Celsius); type Celsius float64
go k(0.1)         // func k(x float32)
go f(1e23)        // func f(x float64)
```

```cpp
double _eo_go_arg_0_0 = double{0.0};
double _eo_go_arg_1_0 = double{0x1.5555555555555p-02};
double _eo_go_arg_2_0 = double{6.0};
int64_t _eo_go_arg_3_0 = int64_t{5};
Celsius _eo_go_arg_4_0 = Celsius{37.5};
float _eo_go_arg_5_0 = float{0x1.99999ap-04f};
double _eo_go_arg_6_0 = double{0x1.52d02c7e14af6p+76};
```

Because of the `T{L}` form, the constant's C++ type is always `T`, even when `T` is a defined type whose literals would otherwise have the underlying type. The C++ mapping of Go defined types is not frozen; `T{L}` applies to whatever type it produces. Goroutine temporaries are declared with the parameter's value type, as for any argument. A defer temporary may stay `auto` when `T` is the parameter's value type; for an interface parameter, where `T` is the default type, declare the defer temporary with the parameter's value type, so that the conversion happens at the defer statement.

### Strings

Go `string` maps to `std::string`. A Go string is immutable, so a copy of a `std::string` behaves like Go's shared bytes, and only the cost differs. The functions below are in `<eo/string.h>`, which `<eo/core.h>` includes. They cover the operations whose plain C++ form would differ from Go's:

| Go | C++ |
| --- | --- |
| `string` | `std::string` |
| an interpreted literal `"..."` | `"..."s`, see [String literals](#string-literals) |
| a raw literal `` `...` `` | `R"(...)"s` |
| `len(s)` | `eo::len(s)`, an `int64_t` byte count |
| `s[i]` | `eo::at(s, i)`, a `uint8_t` |
| `s[low:high]`, `s[low:]`, `s[:high]` | `eo::substr(s, low, high)`, `eo::substr(s, low)`, `eo::substr(s, 0, high)` |
| `s + t`, `s += t` | `s + t`, `s += t` |
| `s == t`, `s < t`, and the other comparisons | the same operators |
| `string(r)`, where `r` has an integer type | `eo::string_from_rune(r)` |

These four helpers are function objects, and a translation always calls them qualified, as `eo::len(s)`. A Go program can declare a function of the same name, such as `substr`, whose spelling the translation keeps (see [Identifier mapping](#identifier-mapping)). The qualified call still reaches Eo. When the Go function is declared at global scope, an unqualified call to it also finds the helper through `using namespace eo;`, so it is ambiguous and does not compile, instead of silently resolving to either one. A Go function declared in a namespace that encloses the call hides the helper, and the call reaches the Go function, as in Go. Such a program needs the identifier rule of [#83](https://github.com/conr2d/eo/issues/83), which also tracks collisions with the other Eo names, such as `New`.

`eo::at` and `eo::substr` accept an index of any Go integer type: any C++ integer type other than `bool`, `char`, `wchar_t`, `char8_t`, `char16_t`, and `char32_t`. An index or bound out of range panics with Go's runtime error, such as `runtime error: index out of range [5] with length 3` or `runtime error: slice bounds out of range [:5] with length 3`, which a deferred `recover()` can stop. `std::string::operator[]` and `std::string::substr` are not used for Go indexing and slicing, because they do not check bounds the same way and `operator[]` yields a `char`, whose signedness differs from Go's `byte`.

`eo::string_from_rune` converts the integer to the UTF-8 encoding of that code point. A value that is negative, above `0x10FFFF`, or a surrogate converts to `"\uFFFD"`, as in Go. A `byte` value converts as a code point too, so `string(byte(0xff))` is the two bytes of U+00FF.

`std::string` comparisons compare bytes as `unsigned char`, as Go does, so `"\xff" > "a"` holds in both.

Pass, return, and store strings by value, as `std::string`, never as `const std::string&`. A reference parameter would see a later assignment to the caller's variable, which a Go parameter does not.

`eo::len` of a string is an `int64_t`, Go's `int`. Arithmetic on the `uint8_t` that `eo::at` returns is converted back to `uint8_t`, as in `uint8_t(eo::at(s, i) - '0')` (see [Arithmetic operators](#arithmetic-operators)).

The conversions between strings and `[]byte` or `[]rune` are in [Slices](#slices), and `for range` over a string is in [Range loops](#range-loops). Not covered yet: the `strings`, `strconv`, and `unicode/utf8` packages.

#### String literals

An interpreted string literal keeps its source text, with two changes: each `\xhh` escape is written `\x{hh}`, and the literal gets the `s` suffix from `std::string_literals`, which `<eo/string.h>` makes visible through `using namespace eo;`:

```go
s := "caf\u00e9 → \xff\x00!"
```

```cpp
auto s = "caf\u00e9 → \x{ff}\x{00}!"s;
```

Only an escape sequence changes, not text that merely looks like one: in `\\x41`, the escape is `\\`, and the C++ literal is `\\x41` too. Go's other escapes, `\a \b \f \n \r \t \v \\ \"`, a three-digit octal `\ooo`, `\uXXXX`, and `\UXXXXXXXX`, mean the same in C++. A carriage return byte written directly in the source text is written `\r`, because a C++ compiler reads it as the end of the line. A C++ `\x` escape without braces takes every hex digit that follows it, so `"\xffc"` would be one out-of-range escape instead of Go's two bytes. The `s` suffix keeps a NUL byte, which `std::string{"..."}` would cut off. Non-ASCII characters are copied as written. `<eo/string.h>` checks that the source encoding and the compiler's ordinary literal encoding are both UTF-8, so `\u00e9` and a literal `é` are Go's bytes.

A raw string literal is `R"(...)"s` with the source text inside, after dropping each carriage return, as Go does. The delimiter is empty unless the text contains `)"`. Otherwise it is the first of `eo`, `eo1`, `eo2`, ... for which the text does not contain `)` followed by the delimiter and `"`, as in `R"eo(...)eo"s`.

A string constant that the Go type checker folds, such as an untyped constant argument (see [Untyped constant arguments](#untyped-constant-arguments)), is written from its value rather than its source: each byte from `0x20` to `0x7e` other than `"` and `\` as itself, `\a \b \f \n \r \t \v \\ \"` for those bytes, and every other byte as `\x{hh}` with two lowercase hex digits.

### Slices

Go `[]T` maps to `slice<T>` from `<eo/slice.h>`, which `<eo/core.h>` includes. A `slice<T>` is a view of a shared backing array: copying it copies the view, and every copy and every slice expression of it refers to the same elements, as in Go. The backing array stays alive for as long as any slice or element pointer refers to it.

| Go | C++ |
| --- | --- |
| `[]T` | `slice<T>` |
| `nil` as a `[]T`, and the zero value of `[]T` | `slice<T>{}`, or `nullptr` where it converts; `var s []T` is `slice<T> s;` |
| `s == nil`, `s != nil` | `s == nullptr`, `s != nullptr` |
| `[]T{a, b, c}`, `[]T{}` | `slice<T>::of({a, b, c})`, `slice<T>::of({})` |
| `make([]T, n)`, `make([]T, n, c)` | `eo::make_slice<T>(n)`, `eo::make_slice<T>(n, c)` |
| `len(s)`, `cap(s)` | `eo::len(s)`, `eo::cap(s)`, as `int64_t` |
| `s[i]` | `eo::at(s, i)`, a `T&` to the shared element |
| `s[low:high]`, `s[low:]`, `s[:high]`, `s[:]` | `eo::subslice(s, low, high)`, `eo::subslice(s, low)`, `eo::subslice(s, 0, high)`, `eo::subslice(s, 0)` |
| `s[low:high:max]`, `s[:high:max]` | `eo::subslice(s, low, high, max)`, `eo::subslice(s, 0, high, max)` |
| `append(s, a, b)`, `append(s)` | `eo::append(s, {a, b})`, `eo::append(s, {})` |
| `append(s, t...)` | `eo::append_slice(s, t)` |
| `append(b, str...)`, where `b` is a `[]byte` | `eo::append_slice(b, str)` |
| `copy(dst, src)`, `copy(b, str)` | `eo::copy(dst, src)`, `eo::copy(b, str)`, an `int64_t` count |
| `clear(s)` | `eo::clear(s)` |
| `[]byte(str)`, `string(b)` | `eo::bytes_from_string(str)`, `eo::string_from_bytes(b)` |
| `[]rune(str)`, `string(r)`, where `r` is a `[]rune` | `eo::runes_from_string(str)`, `eo::string_from_runes(r)` |
| a variadic parameter `xs ...T` | a parameter `slice<T> xs` |
| `f(a, b)`, `f()`, `f(t...)` for a variadic `f` | `f(slice<T>::of({a, b}))`, `f(nullptr)`, `f(t)` |

`byte` is `uint8_t` and `rune` is `int32_t`. A variadic call passes the slice where Go passes it: arguments after the fixed ones become a new slice, no arguments pass nil, and `t...` passes `t` itself, so the callee shares its elements.

`slice<T>{}` is the nil slice, because `T{}` is the zero value of every type in Eo. A composite literal is never nil, even when it is empty, so it is written `slice<T>::of({...})`. A nested literal whose element type Go elides spells it, and a `nil` element is `nullptr`: `[][]int{{1}, nil, {}}` is `slice<slice<int64_t>>::of({slice<int64_t>::of({1}), nullptr, slice<int64_t>::of({})})`. The braced list evaluates the elements left to right, and so does the braced list of `eo::append`, but the list is itself an argument of a function call, which C++ does not sequence against the other arguments. A Go call in a literal or in an append therefore follows [Calls next to slice and string operands](#calls-next-to-slice-and-string-operands), like a call in any other operand of a helper.

Slicing a nil slice gives nil, and slicing any other slice gives a non-nil one, even when it is empty, as in Go. Appending nothing returns the slice unchanged, so `append(s)` and `append(s, t...)` with an empty `t` keep a nil `s` nil.

`eo::at`, `eo::subslice`, and `eo::make_slice` accept the same Go integer types as `eo::at` on a string (see [Strings](#strings)). An index out of range panics with Go's runtime error. A slice's high bound is checked against its capacity, so slicing a slice of capacity 3 as `s[:5]` panics with `runtime error: slice bounds out of range [:5] with capacity 3`, and the three-index form checks `max`, then `high`, then `low`, with Go's messages, such as `slice bounds out of range [:3:2]`. `make` panics with `runtime error: makeslice: len out of range` for a negative length, and `makeslice: cap out of range` for a capacity below the length. A length or capacity whose storage exceeds Go's 2<sup>48</sup> byte allocation limit panics in the same way, measured with the C++ size of `T`, which can differ from Go's.

`eo::at` returns a reference into the backing array, so `s[i] = v` is `eo::at(s, i) = v`, and `s[i] += v` is `eo::at(s, i) += v`. C++ evaluates the right operand of an assignment before the left, so `eo::at(s, i) = f()` calls `f` before it checks `i`, as the gc compiler does. When the left side also contains a call, as in `s[g()] = f()`, the rule below applies. Do not bind the reference to a C++ reference variable: an `append` past capacity or an assignment to `s` can leave it referring to an array that is no longer shared or no longer alive. Take the address with the pointer form, `ptr(s, &eo::at(s, i))`, instead (see [Pointer type](#pointer-type)).

`append` writes in place when the new elements fit in the capacity of `s`, so other slices of the same backing array observe them, as in Go. Otherwise it copies the elements into a new array, and the result no longer shares elements with `s`. The capacity of the new array follows the formula of Go's runtime, `nextslicecap`, without its rounding to allocation size classes. Go does not specify this capacity, and the gc compiler's choice depends on the element size and on escape analysis: a slice that does not escape may start in a 32-byte stack buffer. A program whose behavior depends on the capacity after a reallocating `append` is outside this mapping, and parity cases do not print it. `copy` and `eo::append_slice` handle overlapping elements as Go does, as if through a temporary buffer.

Pass, return, and store slices by value, as `slice<T>`, never as `const slice<T>&`, for the same reason as pointers. A slice variable is boxed by the rules in [Boxed variables](#boxed-variables), like any other variable. A function literal that captures a slice by value still shares its elements.

`fmt::Println` prints a slice as Go does: its elements in brackets separated by spaces, `[]` for nil, and a `[]byte` as numbers.

`begin()` and `end()` give the elements, so std algorithms work over a slice, as in `std::sort(s.begin(), s.end())`. They are not a translation of a Go form.

The slice helpers are function objects with the same collision rule as the string helpers (see [Strings](#strings)).

`for range` over a slice is in [Range loops](#range-loops). Not covered yet: keyed elements in a composite literal, such as `[]T{2: x}`, slicing an array or a pointer to an array, and converting a slice to an array or to a pointer to an array, which wait on a mapping of Go arrays ([#93](https://github.com/conr2d/eo/issues/93)); the `slices` package ([#94](https://github.com/conr2d/eo/issues/94)); and variadic calls in `go` and `defer` statements ([#95](https://github.com/conr2d/eo/issues/95)).

#### Calls next to slice and string operands

C++ leaves the order of a function's arguments unspecified, and every `eo::` helper is a function call. Go calls functions and methods in lexical left-to-right order, but it does not specify when the other operands of an expression are read relative to those calls. The translation must not leave that order to the C++ compiler. A reference that `eo::at` returns before a call that frees its backing array refers to freed storage, and the two orders can print different values.

Eo fixes one order: an expression's nested Go calls run first, left to right, and the operands that remain in the statement are read after them. When an expression contains a call to an `eo::` slice or string helper, a call to `slice<T>::of`, or a keyed struct literal whose keys are out of declaration order (see [Composite literals](#composite-literals)), save the result of each nested Go call in a temporary, `auto _eo_call_<n> = ...;`, immediately before the statement, and use the temporary in its place:

```go
fmt.Println(s[0], g())
s[0] = s[1] + g()
x := f(s[0], g())
s[g()] = h()
```

```cpp
auto _eo_call_0 = g();
fmt::Println(eo::at(s, 0), _eo_call_0);
auto _eo_call_1 = g();
eo::at(s, 0) = eo::at(s, 1) + _eo_call_1;
auto _eo_call_2 = g();
auto x = f(eo::at(s, 0), _eo_call_2);
auto _eo_call_3 = g();
eo::at(s, _eo_call_3) = h();
```

The rule applies separately to each full expression that a statement evaluates: an expression statement, each side of an assignment, the initializer of a declaration, the operand of a `return`, and the condition of an `if` or the tag of a `switch` without an init statement, whose temporaries go before the `if` or `switch`:

- Every Go function or method call in the full expression is a nested call, except the outermost operation when that operation is itself a Go call. This includes the arguments of an `eo::` helper, the elements of a braced list in `slice<T>::of` or `eo::append`, the elements of a struct literal, operands of operators and conversions, parenthesized calls, and the call that produces the function value of another call, as `next()` in `next()(s[0], g())`. The outermost call stays in place, such as `fmt.Println`, `f`, and `h` above, because C++ evaluates its arguments before it runs. C++ evaluates the right side of an assignment before the left, so in `s[g()] = h()` above, `h()` runs after the saved `g()` and before the index check, as Go orders them. When the outermost operation is a helper, an operator, or a conversion, every Go call in the expression is nested, except inside an operand of `&&` or `||` (see below): `t := []int{f(), h(s[0], g())}` becomes `auto _eo_call_0 = f();`, `auto _eo_call_1 = g();`, `auto _eo_call_2 = h(eo::at(s, 0), _eo_call_1);`, then `auto t = slice<int64_t>::of({_eo_call_0, _eo_call_2});`, and `s = append(s, g())` becomes `auto _eo_call_3 = g();`, then `s = eo::append(s, {_eo_call_3});`.
- Builtins and conversions, such as `len`, `cap`, `append`, `string(b)`, and `int64(x)`, are not themselves Go calls for this rule, but the Go calls inside them are. A call inside a function literal's body belongs to that body.
- A temporary's initializer is an expression in its own right. If it contains a helper and a nested Go call, that call is saved first. Temporaries are declared in the order Go makes their calls. `<n>` counts them from 0 through the enclosing function, statement by statement in source order and, within one statement, in call order, with the bodies of function literals numbered where they appear in the source, so no name is reused. `fmt.Println(f(s[0], k()), g())` becomes `auto _eo_call_0 = k();`, then `auto _eo_call_1 = f(eo::at(s, 0), _eo_call_0);`, then `auto _eo_call_2 = g();`.
- Go evaluates the right operand of `&&` and `||` only when the left one does not decide the result, so a call is never moved out of either operand. Instead, a `&&` or `||` that contains a Go call and is nested in the expression is itself saved as one temporary, in its place in call order: `fmt.Println(s[0], f() && h(), g())` becomes `auto _eo_call_0 = f() && h();`, then `auto _eo_call_1 = g();`. Each operand is an expression in its own right, whose outermost call stays in place, so `i < len(t) && check(t[i])` needs no temporary. An operand that would need one is not covered yet.
- In a `go` or `defer` statement, the saved calls come before the statement's function value, receiver, and argument temporaries.
- An expression that no statement can precede is not covered yet: a loop condition or post statement, an `else if` condition, the condition of an `if` or `switch` with an init statement, and a `switch` case expression.
- Also not covered yet, when the expression uses a helper or contains a keyed struct literal out of declaration order: a send statement, whose channel and value operands C++ does not sequence; a receive operation, which Go orders with the calls; and a call that produces the receiver of a method call or the operand of a selector, as `p()` in `p().M(s[0], g())`, `p().f = s[0] + g()`, or `*p() = s[0] + g()`. The general evaluation-order rule that would cover these, and expressions without helpers, is [#96](https://github.com/conr2d/eo/issues/96).

The gc compiler does not always follow Eo's order. It reads `s`, `s[i]`, and a slice or string variable after the calls, as Eo does. It evaluates `len`, `cap`, slice expressions, and string indexing of a variable that are written before a call ahead of that call, so `fmt.Println(len(s), g())`, where `g` assigns `s`, prints the old length. The Go specification allows both orders, so a program whose output depends on this order is outside the mapping, and parity cases do not depend on it.

`ptr(s, &eo::at(s, i))` names `s` twice, so `s` must be a variable, or a field or element path of one without calls; a slice operand with a call is saved by the rule above first, so that the pointer's owner and target are the same array.

### Structs

A Go struct type maps to a C++ aggregate of the same name. Its fields keep their names, order, and grouping, each with the translated type and the initializer `{}`, so a struct's fields start as their zero values and `T{}` is the zero value of `T`. Field tags are dropped. After the fields, every struct declares a defaulted `operator==`, and then its methods as static member functions, in the order Go declares them (see [Methods](#methods)):

```go
type Point struct {
	X, Y int
}

type Labeled struct {
	Name string `json:"name"`
	At   Point
	Tags []string
	Next *Labeled
}
```

```cpp
struct Point {
  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Labeled {
  std::string Name{};
  Point At{};
  slice<std::string> Tags{};
  ptr<Labeled> Next{};

  auto operator==(const Labeled&) const -> bool = default;
};
```

| Go | C++ |
| --- | --- |
| `type T struct { ... }` | `struct T { ... };` |
| `var x T` | `T x;`, as for a slice or a pointer, or `auto x = New<T>();` when `x` is [boxed](#boxed-variables) |
| the zero value of `T`, `T{}` | `T{}` |
| `T{a, b}` | `T{a, b}` |
| `T{f: a, g: b}` | `T{.f = a, .g = b}`, with the fields in declaration order |
| `&T{...}` | `New<T>(T{...})` |
| `x.f`, `p.f` where `p` is a pointer | `x.f`, `p->f` |
| `x == y`, `x != y` | `x == y`, `x != y` |
| `struct{}`, `struct{}{}` | `eo::empty_struct`, `eo::empty_struct{}` |

Assigning, passing, and returning a struct copies its fields, as in Go. A pointer or slice field of the copy shares its target or backing array with the original.

The defaulted `operator==` compares the fields in declaration order and stops at the first pair that differs, as Go does. If a field's type has no `==` in Go, such as a slice, the operator is deleted, so a comparison does not compile, as it does not in Go. Clang warns about such a deleted operator with `-Wdefaulted-function-deleted`. The warning is expected.

#### Composite literals

A positional literal `T{a, b}` lists every field in order, as Go requires, and is the same aggregate initialization in C++. A keyed literal uses designated initializers, which C++ requires in declaration order, so the keys are reordered when Go lists them in another order. An omitted field keeps its `{}` zero value. A literal whose type Go elides spells it, as in [Slices](#slices): `[]Point{{1, 2}, {Y: 3}}` is `slice<Point>::of({Point{1, 2}, Point{.Y = 3}})`, and `[]*Point{{1, 2}}` is `slice<ptr<Point>>::of({New<Point>(Point{1, 2})})`.

C++ evaluates the initializers of a braced list from left to right, so a literal whose keys are in declaration order makes its own calls in Go's order. Their order relative to the other arguments of an enclosing call is the general evaluation-order question of [#96](https://github.com/conr2d/eo/issues/96), as for any argument. A keyed literal with keys out of declaration order would make its own calls in another order. An expression that contains such a literal follows [Calls next to slice and string operands](#calls-next-to-slice-and-string-operands): each nested Go call in the expression is saved in an `_eo_call_<n>` temporary, in Go's order, before the statement:

```go
r := Point{Y: f(), X: g()}
show(k(), Point{Y: f(), X: g()})
```

```cpp
auto _eo_call_0 = f();
auto _eo_call_1 = g();
auto r = Point{.X = _eo_call_1, .Y = _eo_call_0};
auto _eo_call_2 = k();
auto _eo_call_3 = f();
auto _eo_call_4 = g();
show(_eo_call_2, Point{.X = _eo_call_4, .Y = _eo_call_3});
```

#### Declaration order

Go resolves the package-level types of a file in any order, but C++ needs a struct to be complete before another struct holds it by value. A translated file therefore has:

1. a forward declaration `struct T;` of every struct type the file declares, in source order;
2. the struct definitions, in source order, except that a struct that another struct holds by value, as a field or an embedded field, is defined first. Apply this recursively: before defining a struct, define each struct it holds by value that is not defined yet, in the order of its fields. A pointer or slice field needs only the forward declaration;
3. a prototype of each function that is called above its definition, in source order;
4. the remaining declarations in source order, including the out-of-class definitions of methods where Go declares the methods.

A comment on a type declaration moves with its definition. A struct type declared inside a function is a local struct at the place of its declaration. Go allows no methods on it, so its definition holds only the fields and `operator==`.

#### Embedded fields

An embedded field is a member named after its type, without a package qualifier or `*`: `A` is `::A A{};`, `*C` is `ptr<::C> C{};`, and `pkg.T` is `pkg::T T{};`. The type is qualified because a member has its name (see [Package-level names hidden by members](#package-level-names-hidden-by-members)).

A promoted field or method is written with the full path to it, the one that `go/types` records in `Selection.Index()`. A field reached through an embedded pointer uses `->`, which panics on nil, as Go does. A promoted method call is then an ordinary method call on that path (see [Methods](#methods)):

```go
type Item struct {
	Base
	*Counter
	Name string
}

it.ID = 2          // Base.ID
it.Rename("x")     // func (b *Base) Rename(name string)
it.Inc()           // func (c *Counter) Inc() int
it.Get()           // func (c Counter) Get() int
p.Rename("y")      // p is a *Item
```

```cpp
it.Base.ID = 2;
Base::Rename(ref(it.Base), "x"s);
Counter::Inc(it.Counter);
Counter::Get(*it.Counter);
Base::Rename(ptr(p, &p->Base), "y"s);
```

Here `it` is not boxed. A boxed `it` gives `ptr(it, &it->Base)`, by the pointer table in [Pointer type](#pointer-type). An embedded field hidden by a shallower field of the same name, such as `Item.Name` hiding `Base.Name`, is reached by its path, `it.Base.Name`, as in Go.

#### Printing

`fmt::Println` prints a struct as Go's `%v` does: its fields in braces, separated by spaces, with nested structs, slices, and maps printed the same way and a nil pointer as `<nil>`. An operand that is a pointer to a struct, slice, or map prints as `&` followed by its target, as `&{1 2}`. A pointer inside another value prints as an address, which differs from Go's, so parity cases do not print one. The fields are visited with Boost.PFR, which needs a struct to be an aggregate without base classes, as every translated struct is.

#### Struct forms not covered yet

- a struct with a `String` or `Error` method, which `fmt` prints by calling the method; embedded interfaces; method sets of embedded fields, which decide interface satisfaction; and comparison of interface fields, which can panic ([#89](https://github.com/conr2d/eo/issues/89));
- defined types that are not structs, such as `type Celsius float64` ([#104](https://github.com/conr2d/eo/issues/104)), array fields ([#93](https://github.com/conr2d/eo/issues/93)), and structs as map keys ([#88](https://github.com/conr2d/eo/issues/88));
- anonymous struct types other than `struct{}`, conversions between struct types, and blank fields `_`;
- a method named after its own struct, which C++ does not allow, and a local struct type hidden by a member or a local variable where the translation spells it (see [Package-level names hidden by members](#package-level-names-hidden-by-members));
- generic struct types, such as `type List[T any] struct`, and type aliases, such as `type A = B`;
- struct types declared in other files of the same package, which wait on a mapping of packages;
- method values and method expressions, including promoted ones.

### Range loops

A `for range` loop over an integer, a slice, or a string maps to a C++ range-based `for` statement over `eo::range(x)`, from `<eo/range.h>`, which `<eo/core.h>` includes. C++ evaluates `eo::range(x)` once, before the first iteration, as Go evaluates the range expression, and the result keeps its own copy of `x`.

| Go | C++ |
| --- | --- |
| `for i := range n`, where `n` is an integer | `for (auto i : eo::range(n))` |
| `for i, v := range s`, where `s` is a slice | `for (auto [i, v] : eo::range(s))` |
| `for _, v := range s` | `for (auto [_eo_range_key, v] : eo::range(s))` |
| `for i := range s` | `for (auto i : eo::range(eo::len(s)))` |
| `for i, r := range str`, where `str` is a string | `for (auto [i, r] : eo::range(str))` |
| `for _, r := range str` | `for (auto [_eo_range_key, r] : eo::range(str))` |
| `for i := range str` | `for (auto [i, _eo_range_value] : eo::range(str))` |
| `for range x` | `for ([[maybe_unused]] auto _eo_range_key : eo::range(x))`, with `eo::len(s)` in place of a slice `s` |

A blank variable is the same as an omitted one, so `for i, _ := range s` is `for i := range s`, and `for _ = range x` and `for _, _ = range x` are `for range x`. Where the clause needs a C++ name that the Go source does not give, it uses the hidden name of the position: `_eo_range_key` for the first variable and `_eo_range_value` for the second. A clause never declares the same hidden name twice.

- An integer `n` may have any Go integer type, and `i` has the same type. It takes the values 0 through `n-1`, and none when `n` is 0 or negative. An untyped constant `n` is written `T{L}`, as in [Untyped constant arguments](#untyped-constant-arguments), where `T` is the constant's default type, or the type of the variable in the assignment form below: `for i := range 3` is `for (auto i : eo::range(int64_t{3}))`, and `for i := range 'c' - 'a'` is `for (auto i : eo::range(int32_t{2}))`, because the default type of a rune constant is `rune`.
- For a slice, `i` is an `int64_t` and `v` is a copy of the element. `eo::range(s)` holds a copy of `s`, so the number of iterations is `len(s)` when the loop starts: assigning `s` or appending to it in the body does not change the loop. Each element is read from the original backing array when its iteration starts, so the loop sees what the body wrote to a later element, as in Go. `for i := range s` reads no element, as in Go, so it ranges over `eo::len(s)`.
- For a string, `i` is the `int64_t` byte index of each UTF-8 sequence and `r` is its code point as an `int32_t`. A byte that does not start a valid sequence gives U+FFFD and advances one byte, as `[]rune(str)` does (see [Slices](#slices)). `eo::range(str)` holds a copy of `str`, so assigning `str` in the body does not change the loop.

C++ declares the variables of a range-based `for` again for each iteration, as Go 1.22 does, so the loop's own update is not a write to the previous iteration's variables. A function literal that captures a range variable by reference and is not boxed captures it with `&`, as in `[=, &v]`, which C++20 allows for a structured binding.

`break` and `continue` are C++ `break` and `continue`. The range expression is the argument of `eo::range` and follows [Calls next to slice and string operands](#calls-next-to-slice-and-string-operands) as if it were the initializer of a declaration, so its outermost call stays inside `eo::range(...)`, as in `for (auto [_eo_range_key, v] : eo::range(next()))`. A nested Go call in the range expression is saved before the `for` statement, as in `for _, v := range next()[g():]`, which is `auto _eo_call_0 = next();`, `auto _eo_call_1 = g();`, and `for (auto [_eo_range_key, v] : eo::range(eo::subslice(_eo_call_0, _eo_call_1)))`.

#### Boxed range variables

A variable that a `range` clause declares is not boxed by rule 2 of [Boxed variables](#boxed-variables), because it is declared in the loop's own clause. When the escape facts report it moved to heap, the clause declares its value under the hidden name of its position, and the first statements of the body box it, in clause order:

```go
var values []*int
for _, v := range []int{1, 2, 3} {
	values = append(values, &v)
}
```

```cpp
slice<ptr<int64_t>> values;
for (auto [_eo_range_key, _eo_range_value] : eo::range(slice<int64_t>::of({1, 2, 3}))) {
  auto v = New<int64_t>(_eo_range_value);
  values = eo::append(values, {v});
}
```

Each iteration boxes a new variable, so every pointer keeps its own value, as in Go. The box is made before the rest of the body, so `continue` cannot skip it.

#### Assigning range values

In the assignment form, `for k, v = range x`, the clause declares `_eo_range_key` and `_eo_range_value`, and the first statements of the body assign them to the variables, in order:

```go
for k, w = range words {
}
```

```cpp
for (auto [_eo_range_key, _eo_range_value] : eo::range(words)) {
  k = _eo_range_key;
  w = _eo_range_value;
}
```

A boxed variable is assigned as `*k = _eo_range_key;`, and a blank one is not assigned. The same variable may appear on both sides, as in `for i, i = range s`, and is assigned twice, in order, as in Go. After the loop, the variables hold the values of the last iteration, and a loop with no iterations leaves them unchanged, as in Go. A left operand that is not a variable, such as `s[i]`, `p.f`, or `*p`, is not covered yet.

#### Clause variables redeclared in the body

A loop body may declare a variable with the name of a clause variable, as in the idiom `v := v`. C++ rejects that declaration directly in the body, and its initializer would refer to itself at any depth, so it is renamed as described in [Redeclared names](#redeclared-names):

```go
for _, v := range s {
	v := v * 10
	fmt.Println(v)
}
```

```cpp
for (auto [_eo_range_key, v] : eo::range(s)) {
  auto _eo_shadow_v = v * 10;
  fmt::Println(_eo_shadow_v);
}
```

Not covered yet: `range` over a channel ([#97](https://github.com/conr2d/eo/issues/97)), a map ([#88](https://github.com/conr2d/eo/issues/88)), an array or a pointer to an array ([#93](https://github.com/conr2d/eo/issues/93)), and a function ([#98](https://github.com/conr2d/eo/issues/98)), and labeled `break` and `continue` (see [Unfrozen constructs](#unfrozen-constructs)).

### Pointers and escaping variables

Go keeps a variable alive for as long as anything can reach it, and a function literal or goroutine that uses a variable shares it with the function that declared it. A C++ local is destroyed at the end of its block, and a lambda that captures by value copies the variable. This mapping gives such a variable shared heap storage that stays alive while any pointer refers to it. The Go compiler, not the translator, decides which variables need it.

#### Pointer type

Go `*T` maps to `ptr<T>` from `<eo/ptr.h>`, which `<eo/core.h>` includes:

| Go | C++ |
| --- | --- |
| `*T` | `ptr<T>` |
| `nil` as a `*T`, and the zero value of `*T` | `ptr<T>{}`, or `nullptr` where it converts |
| `p == nil`, `p != nil` | `p == nullptr`, `p != nullptr` |
| `p == q` | `p == q` |
| `*p` | `*p` |
| `p.f` | `p->f` |
| `new(T)` | `New<T>()` |
| `&T{...}` | `New<T>(...)` with the translated composite literal as the argument |
| `&x`, where `x` is [boxed](#boxed-variables) | `x` |
| `&x.f`, where `x` is boxed | `ptr(x, &x->f)` |
| `&x`, `&x.f`, where `x` is not boxed | `ref(x)`, `ref(x.f)` |
| `&p.f`, where `p` is a pointer | `ptr(p, &p->f)` |
| `&*p` | `p` |
| `&p[i]`, where `p` is a pointer to an array | `ptr(p, &(*p)[i])` |
| `&s[i]`, `&s[i].f`, where `s` is a slice | `ptr(s, &eo::at(s, i))`, `ptr(s, &eo::at(s, i).f)` |

`&x.f` stands for the address of any part of `x`: `x` followed by field selectors and array indexes that stay inside `x`'s own storage, such as `x.f[i].g`. The `ptr(owner, address)` form keeps the whole target of `owner` alive, as Go does for a pointer into the middle of a variable. For a slice, the owner is its backing array, so `&s[i]` stays valid after `s` is assigned or goes out of scope, and still refers to the old array after an `append` past capacity, as in Go.

A pointer from `New` owns its target, and every copy shares it. The target is destroyed when the last pointer to it is gone. A pointer from `ref` does not own its target. The rules below use `ref` only for storage that outlives every use of the pointer: package-level variables, and locals that are not boxed.

Pass, return, and store pointers by value, as `ptr<T>`, never as `const ptr<T>&`. A reference to a pointer variable would see a later assignment to that variable, which a Go parameter does not. If that assignment dropped the last owner, the reference would also refer to a destroyed target. Copying an owning pointer increments an atomic count. Copying a `ref` pointer does not.

`*p`, `p->f`, and `ptr(p, &p->f)` on a nil `p` panic with Go's runtime error `runtime error: invalid memory address or nil pointer dereference`. A deferred `recover()` can stop it, as in Go. Otherwise the program ends with status 2. Go also prints a signal line to stderr, which is not part of parity.

A reference cycle of owning pointers is never destroyed, while Go's garbage collector frees it. Eo does not provide weak pointers. The Go source does not record which pointer in a cycle could be weak, and a wrong choice would destroy a target that is still reachable. A program that keeps creating unreachable cycles uses more memory under Eo than under Go, but it behaves the same otherwise (see [#80](https://github.com/conr2d/eo/issues/80)).

Destroying a target releases the pointers it holds without recursion, so dropping the head of a long linked list frees it without overflowing the stack.

A data race on a pointer variable, such as one goroutine assigning `p` while another copies it, is undefined behavior in Eo, as it is for any variable outside the [data-race-free programs](./TRANSLATION.md) that Eo targets. Go happens to keep a racy single-word pointer write memory-safe; a reference-counted pointer does not.

#### Escape facts

Build the package being translated with the Go toolchain that its module selects, and keep the compiler's escape report:

```sh
go build -gcflags='all=-l -m=2' <package>
```

Two kinds of lines are used, each naming a variable by its declaration position:

- `moved to heap: x`: the variable can outlive its function's frame, or is shared with code that can;
- `<function> capturing by value: x` and `<function> capturing by ref: x`: how a function literal captures `x`. Go captures by value only a variable that is never written after it is captured, so the copy always equals the variable.

Ignore capture lines for compiler temporaries, whose names start with `.autotmp_`. A capture line names the function that declares the literal, and the position of the captured variable's declaration.

`-l` turns off inlining. With inlining, the compiler can inline a function literal and drop its capture lines, so the facts would depend on its inlining decisions. Without inlining, a few variables may be moved to heap that an inlining build keeps on the stack. Boxing them costs an allocation, and the translation stays correct.

#### Boxed variables

A local variable or parameter `x` is boxed if:

1. the escape facts report `moved to heap: x`; or
2. `x`'s address is taken, or a function literal captures `x` by reference, and `x` is declared anywhere except directly in the function body before the `try` block of the [defer shape](#defer), in a loop's own clause, or directly in a loop body. This includes the body of an `if`, `else`, `switch` case, or `select` case, a bare block `{ ... }`, the init statement of an `if` or `switch`, a type switch guard, a `select` receive such as `case v := <-ch:`, and the top level of the `try` block, at any depth.

A variable declared by a loop's own clause or directly in a loop body is not boxed by rule 2. Go reports a variable moved to heap when its address can outlive one iteration, so any other pointer to it is declared inside the same iteration and is destroyed with it.

Taking the address of `x` means `&x` or `&x.f` on a part of `x`, a call or method value of a pointer-receiver method on `x` or a part of `x` (`x.Lock()`, `x.f.Add(1)`, `x.M`), and a slice expression on `x` or a part of `x` that is an array.

Rule 2 is needed because Go keeps a variable that is not moved to heap in its function's frame until the function returns, while the C++ variable is destroyed at the end of its block:

```go
var p *int
{
	x := 5
	p = &x // Go keeps x on the stack, but p still refers to it below
}
*p += 1
```

```cpp
ptr<int64_t> p;
{
  auto x = New<int64_t>(5);
  p = x;
}
*p += 1;
```

A boxed variable is declared as a pointer to its value, with the translated Go type of the variable as `T`:

| Go | C++ |
| --- | --- |
| `var x T` | `auto x = New<T>();` |
| `x := v`, `var x T = v` | `auto x = New<T>(v);` |
| parameter `x T` | parameter `T _eo_param_x`, and `auto x = New<T>(_eo_param_x);` at the start of the body |
| named result `x T` | `auto x = New<T>();` at the start of the body, in place of the `T x{}` of the defer shape, and `*x` wherever the function returns `x` |

At the start of the body, boxed parameters come first, in parameter order, then result storage, then `eo_defer_scope`.

Every other use of `x` is `*x`, `x.f` is `x->f`, and `&x` is `x`.

#### Loop clause variables

A variable that the init statement of a three-clause `for` loop declares, and that is moved to heap, is boxed in the init statement. Go gives every iteration its own variable: before the post statement runs, it declares the next iteration's variable and copies the current value into it. The C++ post expression therefore first assigns a new box to each boxed clause variable, in declaration order, and then runs the translated post statement, joined by the comma operator:

```go
var head *Node
for i := 0; i < 3; i++ {
	head = &Node{p: &i, next: head}
}
```

```cpp
ptr<Node> head;
for (auto i = New<int64_t>(0); *i < 3; i = New<int64_t>(*i), (*i)++) {
  head = New<Node>(Node{.p = i, .next = head});
}
```

Each node keeps the box of its own iteration, so the values are 0, 1, and 2, as in Go. The condition is first evaluated on the init statement's box, and after that the condition and the post statement act on each iteration's new box. `continue` jumps to the C++ post expression, so the copy happens there too, as it does in Go. A loop without a post statement, `for i := 0; i < n; {`, has only the new-box assignments as its C++ post expression. Several boxed clause variables of one type share the init declaration, as in `for (auto i = New<int64_t>(0), j = New<int64_t>(10); *i < *j; i = New<int64_t>(*i), j = New<int64_t>(*j))`.

A clause variable that is not boxed stays a plain C++ variable that every iteration reuses. Nothing that outlives an iteration refers to it, so the per-iteration copy cannot be observed.

This shape does not cover:

- an init statement that declares variables of different translated types, or a boxed and an unboxed variable, because one C++ `auto` declaration cannot declare them together;
- a post statement that does not translate to a single C++ expression, such as `i, j = i+1, j-1`.

A variable that a `range` clause declares is boxed as described in [Boxed range variables](#boxed-range-variables). A body that declares a variable with the name of a clause variable renames it as described in [Redeclared names](#redeclared-names).

#### Function literals

A function literal translates to a lambda `[=, &a, &b]() mutable`, where `a` and `b` are the variables that the literal captures by reference and that are not boxed. Every other captured variable is copied. A variable that Go captures by value never changes after it is captured, so the copy is equivalent. A copied box is a copy of the pointer, so it shares the variable. Package-level variables are not captured.

```go
x := 0 // captured by reference, not moved to heap
y := 5 // captured by value
inc := func() { x++ }
get := func() int { return x + y }
```

```cpp
auto x = int64_t{0};
auto y = int64_t{5};
auto inc = [=, &x]() mutable { x++; };
auto get = [=, &x]() mutable -> int64_t { return x + y; };
```

By rule 2, a variable captured by reference that is not boxed is declared directly in the function body before any `try` block, in a loop clause, or directly in a loop body. It is alive for every call the literal can receive: Go reports the variable moved to heap if the literal can be called after the function returns, or after the iteration that declared the variable.

Two kinds of literal have their own frozen shape:

- a literal launched by `go` is `[=]() mutable`, as described in [Goroutine](./GOROUTINE.md#function-literals). A goroutine's literal always outlives its frame, so every variable that it captures by reference is moved to heap;
- a deferred literal is `[&]` when every variable it captures is a parameter, result, or variable declared before the `try` block of its function, or a variable of an enclosing function, which the function reaches through its own capture for as long as it runs. Otherwise it is `[&, v...]() mutable`, listing by copy each captured variable that is declared inside the `try` block. Such a variable is captured by value, is boxed by rule 1 or rule 2, or is declared in a loop clause or directly in a loop body, where Go moves it to heap if a deferred literal captures it by reference (see [Defer](#defer)). A boxed loop clause variable is copied as the box of the iteration that registers the call, as in `eo_defer([&, i]() mutable { use(*i); });`.

#### Methods

A method of a translated type `T` is a static member function whose first parameter is the receiver: a `ptr<T>` for a pointer receiver, and a `T` for a value receiver. A value receiver is then an ordinary parameter, which is boxed by the same rules as any other:

```go
func (c *Counter) Start() {
	go c.loop()
}
```

```cpp
struct Counter {
  // fields
  static void Start(ptr<Counter> c);
  static auto loop(ptr<Counter> c) -> func<>;
};

void Counter::Start(ptr<Counter> c) {
  auto _eo_go_receiver_0 = c;
  go([=]() mutable { return Counter::loop(_eo_go_receiver_0); });
}
```

The receiver is not `this`, because a method can keep its receiver after it returns, here in a goroutine, and `this` cannot keep the target alive. A value receiver is not `*this` either: a goroutine or closure that captures it must capture a copy or a box, never `this`.

| Go call | Pointer receiver | Value receiver |
| --- | --- | --- |
| `p.M(args)`, `p` a pointer | `T::M(p, args)` | `T::M(*p, args)` |
| `x.M(args)`, `x` addressable | `T::M(&x, args)`, with `&x` translated as above | `T::M(x, args)` |

Go does not specify whether `*p` in a value-receiver call is evaluated before or after the other arguments, and C++ does not either, so `T::M(*p, f())` is translated as written. As in Go, a pointer-receiver call on a nil pointer passes nil, and only a dereference inside the method panics, while a value-receiver call through a nil pointer panics at the call.

A promoted method is called on the path to its embedded field, as in [Embedded fields](#embedded-fields).

Methods of Eo library types, such as `sync::Mutex` and `sync::WaitGroup`, stay member functions, called as `x.M()` or `p->M()`.

#### Not covered yet

- pointers held inside maps and interfaces, whose own mappings are not frozen, and slicing an array;
- printing a pointer with `fmt` inside another value, which shows an address;
- method values and method expressions, and conversions through `unsafe.Pointer` or `uintptr`.

### Goroutine

Go `go` statements follow the canonical mapping defined in [Goroutine Translation Design](./GOROUTINE.md).

In particular:

- evaluate and save a function-valued callee before its arguments;
- evaluate call arguments in source order in the launching goroutine;
- preserve Go value- vs pointer-receiver binding for method calls;
- keep generated evaluation temporaries adjacent to the source `go` statement;
- declare each argument temporary with the translated parameter's value type (reference and `const`/`volatile` removed), not `auto`; a launch whose translated parameter is a non-owning view type is provisional;
- launch a nullary `[=]() mutable` thunk whose body is only the call of the prepared temporaries, so the callee is invoked after launch even if it is not a lazy coroutine;
- pass a function literal with no parameters, called without arguments, to `go(...)` directly, since its body is the callee body, and save any other function literal as the function value;
- do not move any other source expression into the launched thunk;
- translate addresses, pointers, and captures as described in [Pointers and escaping variables](#pointers-and-escaping-variables), which boxes every variable that a goroutine can reach, so that no launch needs a lifetime check of its own (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary)); a launch that passes a value referring to storage through a type whose mapping is not frozen, such as a slice of a local array, is provisional;
- call a method of a translated type as `T::M(receiver, args...)` with the saved receiver;
- apply Go 1.22 and later per-iteration loop-variable semantics; a loop's own post statement or `range` update is not a write to the variables that its clause declares with `:=`, but it is a write to any other variable.

`go(...)` returns without running the launched call, including when it is called from a running goroutine.

The launched function body begins only after the source `go` statement's function and argument evaluation is complete.

### Select

Go `select` follows the canonical mapping defined in [Select Translation Design](./SELECT.md).

In particular:

- a blocking select uses `co_await select.index()`;
- a select with `default` uses `select.try_index()`;
- the `Select` object is constructed for one execution of one source select;
- receive results are accessed synchronously from the selected case;
- communication clauses keep source order;
- every translated case body is braced;
- communication commits before the selected case body executes.

Do not use an alternate Select shape merely because it is equivalent for a particular program.

### Defer

The following mapping is canonical for defer registration, function returns, panics that run deferred calls, and `recover`.

A translated function containing at least one Go `defer` statement declares one function-scoped defer stack at the beginning of the C++ function body:

```cpp
func<> f() {
  eo_defer_scope;
  // ...
}
```

The stack stores deferred calls but does not execute them from its destructor. Every translated normal exit explicitly executes `eo_defer_run` before leaving the function. This keeps function-scope C++ locals alive while deferred calls execute.

A panic is a C++ exception, so the function also catches it and runs its deferred calls with `eo_defer_panic` (see [Panic and recover](#panic-and-recover)). The full shape of a function with a `defer` statement is:

```cpp
auto f(int64_t x) -> int64_t {
  int64_t _eo_result_0{};
  eo_defer_scope;
  // statements before the first statement that contains a defer
  try {
    // the first statement that contains a defer, and every statement after it
  } catch (...) {
    eo_defer_panic;
  }
  return _eo_result_0;
}
```

- Each result has storage declared before `eo_defer_scope`: a named result uses its translated name, and an unnamed result `N` uses `_eo_result_N`. Both are declared as `T name{}` with the translated result type. A function with more than one result is provisional, as multi-value results have no frozen mapping.
- `try {` opens immediately before the first statement at the top level of the function body that is, or contains, a `defer` statement, and before that statement's argument temporaries. It closes at the end of the function body. A panic before that point has no deferred call to run and leaves the function directly. A `defer` inside a nested function literal belongs to that literal, which gets this shape itself, and does not count for the enclosing function.
- After the catch handler, the function returns its result storage: `return _eo_result_0;`, `co_return _eo_result_0;`, or the named results. A function without results ends with nothing, or with `co_return;` for a coroutine. This return is reached only when a deferred call recovered the panic.

A direct named-function defer evaluates and saves its argument expressions in source order before registration. Temporary names use the source defer ordinal followed by the argument ordinal. Defer statements are counted like `go` statements, as described in [Temporary names](./GOROUTINE.md#temporary-names), with a separate count:

```go
defer cleanup(x(), y())
```

```cpp
auto _eo_defer_arg_0_0 = x();
auto _eo_defer_arg_0_1 = y();
eo_defer([=] { cleanup(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
```

Defer argument temporaries are still declared with `auto`, unlike goroutine argument temporaries, which use the translated parameter type (see [Argument temporary types](./GOROUTINE.md#argument-temporary-types)). An untyped constant argument follows [Untyped constant arguments](#untyped-constant-arguments). Until the defer shape is aligned, any other defer argument whose Go conversion changes its type, such as `nil` or a non-constant value passed to an interface parameter, is provisional.

If the deferred callee is itself a function-valued expression, evaluate and save it before the arguments:

```go
defer next()(x())
```

```cpp
auto _eo_defer_fn_0 = next();
auto _eo_defer_arg_0_0 = x();
eo_defer([=]() mutable { _eo_defer_fn_0(_eo_defer_arg_0_0); });
```

A deferred method call saves the receiver binding before its arguments. The receiver temporary must reflect the Go method receiver kind, including Go's implicit address/dereference rules.

For a value-receiver method, save the receiver value that Go would pass to the method. This includes dereferencing a pointer receiver expression when Go selects a value-receiver method through a pointer:

```cpp
auto _eo_defer_receiver_0 = x;
auto _eo_defer_arg_0_0 = y();
eo_defer([=]() mutable { T::M(_eo_defer_receiver_0, _eo_defer_arg_0_0); });
```

For a pointer-receiver method invoked on an addressable value, preserve Go's implicit `&x` binding by saving a pointer to the original value rather than copying it. The pointer is `&x` translated as described in [Pointers and escaping variables](#pointers-and-escaping-variables), and the call uses the [method form](#methods):

```go
defer x.M(y()) // M has receiver *T, and x is not boxed
```

```cpp
auto _eo_defer_receiver_0 = ref(x);
auto _eo_defer_arg_0_0 = y();
eo_defer([=] { T::M(_eo_defer_receiver_0, _eo_defer_arg_0_0); });
```

If `x` is boxed, the receiver is `x` itself: `auto _eo_defer_receiver_0 = x;`. If the source receiver expression already evaluates to a pointer for a pointer-receiver method, evaluate and save that pointer value directly. Translators must resolve the Go receiver kind from the method declaration instead of inferring it from convenient C++ syntax.

A method of an Eo library type, such as `sync::Mutex`, is a member function. Its receiver binding saves a raw `&x` for a package-level `x` or a variable declared before the `try` block, and otherwise `&x` translated by the pointer table, such as the box for a boxed `x` or `ptr(c, &c->mu)` for `defer c.mu.Unlock()`. The deferred call is `_eo_defer_receiver_0->M(...)`, as in [Mutex](#mutex).

A deferred closure may use a direct reference capture only when every referenced translated object remains alive until the function-level `eo_defer_run` and `eo_defer_panic`:

```go
defer func() { use(x) }()
```

```cpp
eo_defer([&] { use(x); });
```

Parameters, result storage, package-level variables, and locals declared at the top level of the function body before `try {` stay alive for both. A local declared inside the `try` block, including one in an inner block or a loop iteration, is destroyed before the catch handler runs `eo_defer_panic`, and an inner-block local before the drain at a later return. A deferred literal that captures such a local lists it by copy, as in `eo_defer([&, v]() mutable { use(v); });`. The local is captured by value in Go, or it is boxed (see [Function literals](#function-literals)), so the copy keeps Go's semantics. A saved address of such a local, such as a pointer-receiver binding, is the box itself: rule 2 of [Boxed variables](#boxed-variables) boxes a local at the top level of the `try` block or in an inner block, and Go moves a loop-body local to heap when a deferred call keeps its address.

Repeated execution, including inside loops, registers a new callable each time. Registered calls execute in LIFO order when `eo_defer_run` executes. The `eo_defer_scope` declaration belongs to the surrounding translated function, not to a nested block containing the defer statement.

For a void normal return, drain immediately before the return:

```cpp
eo_defer_run;
return;
```

For a value return, evaluate the return expression into the result storage first, then drain, then return the storage:

```go
return result()
```

```cpp
_eo_result_0 = result();
eo_defer_run;
return _eo_result_0;
```

A bare `return` with named results drains and returns the named results. A deferred call may change a named result, because it is still the storage that the function returns.

Coroutine returns follow the same rule:

```cpp
eo_defer_run;
co_return;
```

A function that reaches its end without an explicit return inserts `eo_defer_run` at the natural function exit. Every early normal return must have its own corresponding drain sequence.

### Panic and recover

`panic(v)` translates to `panic(v)` from `<eo/panic.h>`, which throws `eo::Panic`. Its `what()` is `panic: ` followed by the value as Go prints it, and `value()` holds the value. A Go string panic value is stored as `std::string`. Runtime panics raised by Eo are `eo::Panic` too: the channel panics such as `close of closed channel`, `sync: negative WaitGroup counter`, `cannot create context from nil parent`, and `invalid argument to Intn`. Any other C++ exception that leaves a translated call is also a panic. `panic(nil)` translates to `panic(nullptr)` and, as since Go 1.21, raises a panic whose message is `panic called with nil argument`.

While a panic leaves a function that contains a `defer` statement, its catch handler runs `eo_defer_panic`:

- the remaining deferred calls run in LIFO order, after those of the functions it called and before those of its callers;
- a deferred call that panics replaces the running panic, and the remaining deferred calls still run;
- if the last panic was recovered, the function returns its result storage normally, and its caller continues. Otherwise the panic leaves the function.

A panic in a deferred call during `eo_defer_run` leaves `eo_defer_run`, and the same catch handler runs the remaining deferred calls.

`recover()` translates to `recover()`, which returns an `eo::Recovered`:

- `r := recover()` translates to `auto r = recover();`;
- `r == nil` and `r != nil` translate to `r == nullptr` and `r != nullptr`;
- printing `r` with `fmt::Println` prints the panic value as Go does, and `<nil>` for nil;
- `panic(r)` starts a new panic with the recovered value and message, and `panic(r)` with a nil `r` is `panic(nil)`. For a panic raised by a C++ exception other than `eo::Panic`, the new panic keeps the message, and its value is empty.

`recover()` returns the running panic, and stops it, only when it is called by a deferred call that `eo_defer_panic` runs. It returns nil when no panic is running, in a deferred call run by `eo_defer_run`, and after the panic was recovered. Go also returns nil when recover is not called directly by the deferred function. Eo cannot detect that. A `recover()` is frozen when it is in one of these places:

- directly in the body of a deferred function literal, `defer func() { ... recover() ... }()`, but not in a function literal nested in that body;
- directly in the body of a named function or method that the program calls only in `defer` statements.

Any other `recover()` is unsupported, because a call from inside a deferred function would recover where Go returns nil.

The type of a recovered value is provisional beyond comparison with nil and printing: `r.value()` is a `std::any`, and a type assertion such as `r.(error)` has no mapping. `runtime.Goexit` has no mapping.

### Blocking operations

A Go operation that blocks the calling goroutine translates to an awaitable that the translation suspends on with `co_await`. It never blocks the OS thread. A blocked worker thread stops other goroutines from running, and with `EOMAXPROCS=1` it stops all of them, while Go parks only the goroutine.

| Go | C++ | Status |
| --- | --- | --- |
| `select` without `default` | `co_await select.index()` | frozen, see [Select](#select) |
| `wg.Wait()` | `co_await wg.Wait()` | frozen |
| `mu.Lock()` | `co_await mu.Lock()` | frozen, see [Mutex](#mutex) |
| `rw.Lock()`, `rw.RLock()` | `co_await rw.Lock()`, `co_await rw.RLock()` | frozen, see [Mutex](#mutex) |
| `ch <- v` | `co_await (ch << v)` | provisional |
| `v := <-ch` | `auto v = co_await *ch` | provisional |
| `time.Sleep(d)` | `co_await time::Sleep(d)` | provisional, including the duration |

A Go function whose body contains a blocking operation, or a call to a function translated as a coroutine, is translated as a coroutine returning `func<T>`, and every call to it is `co_await`ed. Other functions stay plain C++ functions. Calls through function values and interface methods, whose target is not known statically, are provisional.

Two call sites do not `co_await`:

- the `go` thunk returns the call (`return f(...)`), and `go(...)` runs the returned awaitable as the goroutine body (see [Goroutine](#goroutine));
- a deferred call runs synchronously at `eo_defer_run`, so it cannot suspend. A `defer` is unsupported when its call is a blocking operation or a coroutine, such as `defer wg.Wait()`, and when it defers a function literal whose body would be a coroutine, such as `defer func() { ch <- v }()`: the registered call would create an awaitable and drop it, and the deferred work would never run.

Do not translate a Go lock to `std::mutex` or `std::shared_mutex`. Their `lock()` blocks the OS thread, and a translation can then deadlock where Go does not: with `EOMAXPROCS=1`, or whenever the holder is suspended at a `co_await` while it holds the lock.

`sync.Once` and `sync.Cond` have no mapping yet. `Once.Do` blocks while another goroutine runs the function, and `Cond.Wait` blocks until a signal, so `std::call_once` and `std::condition_variable` would block the worker. A translation that uses either is unsupported (see [#76](https://github.com/conr2d/eo/issues/76)).

Do not use `eo::invoke` in a translation. It blocks the calling OS thread until a coroutine finishes, and it exists for C++ code outside the runtime, such as tests.

`sync.WaitGroup` maps to `sync::WaitGroup` from `<eo/sync.h>`. `Add` and `Done` are plain calls, and `Add` panics with `sync: negative WaitGroup counter` when the counter goes below zero.

Sharing a WaitGroup with goroutines follows [Pointers and escaping variables](#pointers-and-escaping-variables). A local `var wg sync.WaitGroup` that goroutines use through a capture or `&wg` is moved to heap, so it is boxed as `auto wg = New<sync::WaitGroup>();` and used as `wg->Add(1)` and `co_await wg->Wait()`. A `*sync.WaitGroup` is a `ptr<sync::WaitGroup>`. The `sync::WaitGroupPtr` alias for `std::shared_ptr<sync::WaitGroup>` is not a translation mapping.

#### Mutex

`sync.Mutex` and `sync.RWMutex` map to `sync::Mutex` and `sync::RWMutex` from `<eo/sync.h>`:

| Go | C++ |
| --- | --- |
| `mu.Lock()` | `co_await mu.Lock();` |
| `mu.Unlock()` | `mu.Unlock();` |
| `mu.TryLock()` | `mu.TryLock()` |
| `rw.RLock()` | `co_await rw.RLock();` |
| `rw.RUnlock()` | `rw.RUnlock();` |
| `rw.TryRLock()` | `rw.TryRLock()` |

`RWMutex` also has `Lock`, `Unlock`, and `TryLock`, mapped as for `Mutex`. Only `Lock` and `RLock` are blocking operations. The others are plain calls, so a `defer` of `Unlock` or `RUnlock` is allowed. As in Go, a goroutine waiting in `rw.Lock()` keeps new readers out until it has unlocked.

Unlocking a lock that is not held in that mode is a fatal error, as in Go. The runtime writes `fatal error: ` and the message to stderr, and exits with status 2:

- `sync: unlock of unlocked mutex`;
- `sync: Unlock of unlocked RWMutex`;
- `sync: RUnlock of unlocked RWMutex`.

Deferred calls do not run, and `recover()` cannot stop a fatal error. When the fatal error happens in a deferred call during a panic, Go also prints that panic first and Eo does not. Stderr is not part of parity, as with panic reports.

`mu.Lock(); defer mu.Unlock()` follows [Defer](#defer). `Unlock` has a pointer receiver, so the defer saves `&mu`. `co_await mu.Lock()` comes before `try {`, because it does not contain the defer:

```go
var mu sync.Mutex
var count int

func inc() {
	mu.Lock()
	defer mu.Unlock()
	count++
}
```

```cpp
func<> inc() {
  eo_defer_scope;
  co_await mu.Lock();
  try {
    auto* _eo_defer_receiver_0 = &mu;
    eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
    count++;
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
```

`inc` contains a blocking operation, so it is a coroutine, and its callers `co_await inc()`.

Holding a lock across a blocking operation, such as a channel send, needs nothing extra: goroutines waiting in `Lock` are suspended, so the holder can still run.

The following are not frozen:

- the C++ form of package-level variables and struct fields, as used in `tests/parity/mutex_counter`. Sharing a local Mutex or RWMutex follows the WaitGroup rules above and is frozen, as in `tests/parity/mutex_local_shared`;
- `RLocker`, and passing a lock as a `sync.Locker`, have no mapping;
- `sync::Mutex`, `sync::RWMutex`, and `sync::WaitGroup` cannot be copied or moved. A Go program that copies one, which `go vet` reports, is unsupported.

### Program termination

A translated program links `eo::main` and defines `func<> eo_main()` in place of Go's `main`. The runtime ends the process the way Go does:

- when `eo_main` completes, the process exits with status 0 at once, without waiting for other goroutines. If another goroutine has an `eo::Panic` that is not yet recovered, the runtime first waits up to about 100 ms for it, as Go's `runtime.main` yields to goroutines running deferred calls for a panic. Another C++ exception counts only while one function runs its deferred calls for it, not while it passes between functions. If a panic is then being reported, it ends the process with status 2 instead;
- `os.Exit(code)` translates to `os::Exit(code)` from `<eo/os.h>`. It ends the process at once from any goroutine, and deferred calls do not run. Do not translate it to `std::exit`: that runs static destructors, and the runtime's thread pool destructor then waits forever for its worker threads;
- an exception that escapes `eo_main`, or the call launched by `go(...)`, is an unrecovered panic. The panicking goroutine's deferred calls have already run in its catch handlers (see [Panic and recover](#panic-and-recover)). The runtime reports the panic on stderr and exits with status 2;
- `fmt::Println` and `fmt::Fprintln` write and flush each call, as Go writes unbuffered to `os.Stdout` and `os.Stderr`. Output is not lost when the process is killed or exits.

Each exit flushes `std::cout`, `stdout` and `stderr`, then ends the process with `std::_Exit`. A stream whose lock another thread holds is skipped, for example while a goroutine is blocked writing to a pipe that nobody reads; that write is lost, as it would be in Go. Other `FILE` streams are not flushed, and no static destructors run.

With `EOMAXPROCS=1`, a goroutine blocked in a read or write occupies the only worker thread, so the goroutine that would call `os::Exit` or return from `eo_main` never runs. Go hands the processor to another thread during a blocking system call. See [#71](https://github.com/conr2d/eo/issues/71).

## Unfrozen constructs

If no canonical translation rule exists for a Go construct, do not silently invent a project-wide convention and treat it as stable.

A translator may use a local adaptation when necessary to make progress, but it should be explicit that the mapping is provisional and may need to change when the translation contract is extended.

Translation-facing mappings with broad source impact should be frozen before large-scale porting depends on them.

Examples currently requiring additional canonical rules include:

- goroutine launches that pass a non-owning view argument, or a value that refers to storage through a type whose mapping is not frozen, such as a slice of a local array (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary));
- pointers held in maps and interfaces, printing a pointer inside another value, method values, and method expressions (see [Not covered yet](#not-covered-yet));
- the struct forms in [Struct forms not covered yet](#struct-forms-not-covered-yet), such as anonymous struct types and blank fields;
- untyped complex constant arguments of `go` and `defer`;
- goroutine launches of method expressions, interface method calls, variadic calls, and multi-value calls, and launches with a nil function value (see [Unfrozen launch forms](./GOROUTINE.md#unfrozen-launch-forms));
- the type of a recovered panic value beyond comparison with nil and printing, and `runtime.Goexit` (see [Panic and recover](#panic-and-recover));
- labeled `break` and `continue`;
- `range` loops over channels, maps, arrays, and functions, and the assignment form with a left operand that is not a variable (see [Range loops](#range-loops)), and the three-clause loop forms that [Loop clause variables](#loop-clause-variables) does not cover;
- zero-value construction for translated types other than basic types, strings, slices, pointers, and structs;
- constant declarations and named constants ([#68](https://github.com/conr2d/eo/issues/68)), and complex numbers;
- panic-producing operations whose naive C++ equivalent would throw differently or invoke undefined behavior.

## Unsupported constructs

Unsupported behavior should fail visibly rather than being approximated silently.

A translator should not replace an unsupported Go construct with a convenient C++ behavior that changes observable semantics.

When a translation cannot be completed under the current Eo contract, the unresolved construct should remain easy to locate and should identify the missing translation rule.

The purpose of this rule is to keep translation gaps explicit so they can drive Eo development instead of becoming hidden semantic drift.

Currently unsupported:

- `defer` of a blocking operation or of a call to a function translated as a coroutine (see [Blocking operations](#blocking-operations));
- `recover()` outside a deferred function literal, unless it is in a named function that is only called in `defer` statements (see [Panic and recover](#panic-and-recover)).
- `sync.Once` and `sync.Cond` (see [Blocking operations](#blocking-operations)), `RWMutex.RLocker`, and passing a lock as a `sync.Locker` (see [Mutex](#mutex));
- copying a `sync.Mutex`, `sync.RWMutex`, or `sync.WaitGroup` (see [Mutex](#mutex)).

## Translation review

A mechanical translation should be reviewable against the Go source primarily by structural comparison.

Review should ask:

1. Does each translated region have an obvious source counterpart?
2. Were source order and control-flow boundaries preserved unless a documented rule required a change?
3. Were expressions evaluated at the same observable time and frequency?
4. Were identifier changes limited to documented cases?
5. Were unsupported semantics made explicit instead of approximated?
6. Were native optimizations kept separate from the mechanical port?

A translation that is semantically correct but unnecessarily restructures the source is not the preferred Eo translation.

Structural review is complemented by behavioral checks. A [parity case](../tests/parity/README.md) runs a Go program and its canonical translation against the same expected output, and records a known divergence as a reproducible test rather than leaving it to review.

## Upstream synchronization

Upstream Go changes should normally be applied by locating the corresponding translated region and reproducing the same structural change under the canonical Eo mappings.

The mechanical C++ baseline should therefore remain close enough to the source that an upstream diff is useful as a guide for the C++ update.

When a native C++ rewrite intentionally diverges from the mechanical structure, that divergence should remain isolated so that upstream semantic changes can still be identified and reconciled deliberately.
