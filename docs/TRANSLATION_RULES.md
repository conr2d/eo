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

When C++ requires a temporary to preserve evaluation order, ownership, lifetime, or coroutine behavior, introduce the smallest local temporary needed and keep it adjacent to the source construct it represents. [Evaluation order](#evaluation-order) gives the temporaries that keep Go's order of calls and receives.

Do not introduce temporaries solely for stylistic reasons.

## Identifier mapping

Preserve Go identifier spelling whenever it is a valid, non-reserved C++ identifier.

If an identifier must change because of C++ syntax, a reserved identifier, a semantic conflict, or an explicit Eo mapping, use the documented mapping rather than inventing a local rename.

A translator must not rename identifiers merely to follow a C++ naming convention.

Names that begin with `_eo_` are reserved for the names that the translation introduces, such as `_eo_call_<n>` and `_eo_switch`. A Go identifier that begins with `_eo_` is not covered.

### Redeclared names

Go lets a local variable hide anything of the same name from an enclosing block or the package: a variable, a constant, a function, or a type. C++ allows the same hiding, with two differences, so a local variable declaration of `x` that hides another `x` is renamed in two cases:

1. The declaration refers to the hidden `x`, at any block depth, as in `x := x`, `x, y := x+1, 0`, `g := g()` for a function `g`, or `point := point{1, 2}` for a type `point`. In C++ the scope of a variable starts at its declarator, so the `x` in its own initializer names the new variable, and `auto x = x;` and `auto g = g();` are ill-formed. An initializer that is written from a constant value does not refer to the hidden name, so `K := K + 1` for a constant `K` is `auto K = int64_t{1025};`.
2. The declaration is directly in the body of a statement whose own clause declares `x`. C++ rejects such a declaration. This covers:
   - the body of a three-clause `for` or a `range` loop whose clause declares `x`;
   - the `if` and `else` branches of an `if` whose init statement declares `x`.

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

Inside a struct definition, and inside the out-of-class definitions of its methods, C++ looks up a name among the struct's members before the package. Go does not: a method sees fields and other methods only through its receiver. C++ also makes a struct definition ill-formed when a name used in it means something else after a member of that name is declared, as in `struct Node { Config Config{}; };`, which GCC rejects. So inside a struct definition and the definitions of its methods, a package-level type, function, variable, or constant whose name is also the name of a member of that struct (a field, an embedded field, or a method) is written with the global qualifier, as `::Config`, or with `_eo_package::`, as `_eo_package::Config`, in a package namespace (see [Package namespaces](#package-namespaces)). An import name that a member hides is qualified in the same way (see [Import names](#import-names)). This includes the struct's own name when a field has it: `type T struct { T int }` declares `auto operator==(const ::T&) const -> bool = default;`. Names that no member shares stay unqualified:

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

A member can also hide an Eo name that `using namespace eo;` makes visible, such as a field named `ptr`, `ref`, `slice`, or `New`. Inside such a struct and its method definitions, that Eo name is written `eo::ptr`, `eo::ref`, and so on (see [#83](https://github.com/conr2d/eo/issues/83) for Eo name collisions in general). A member or package-level name `eo`, `std`, or `boost`, which would hide the qualifier of such a spelling, is not covered.

`::` reaches only package-level names. A struct type declared inside a function is not one, so a local struct type that a member or a local variable hides where the translation spells it has no translation yet: a field or embedded field named after another local type or after its own struct, and a boxed variable named after its local type.

`::` names the global namespace, which holds the declarations of a program of one file. A program of several files puts each package in a namespace of its own, which `_eo_package::` names (see [Packages](#packages)).

### Types named like one of their methods

C++ cannot declare a member with the name of its class, which would be a constructor. A struct or interface type that has a method of its own name takes a trailing underscore, and its `_eo_name` keeps the Go name. The method keeps its name, so a call `e.Error()` does not change, and every other use of the type, such as a declaration, a conversion, or a type assertion, writes the underscore. Go's `runtime.Error`, an interface with an `Error` method, is `runtime::Error_`, and `type Error interface { Error() string; Code() int }` in package `main` is:

```cpp
struct Error_ : interface<Error_> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Error";

  auto Error() const -> std::string;
  auto Code() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Error_::Error, "Error">, eo::method<&Error_::Code, "Code">>;
};
```

A package that also declares a name that ends in an underscore, such as `Error_`, is not covered yet.

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

The helpers return the type of `x`. They are function objects, called qualified, with the same collision rule as the string helpers (see [Strings](#strings)). The `_assign` forms take `x` by reference and evaluate it once, as Go does: `s[i] /= d` is `eo::div_assign(eo::at(s, i), d)`. Their arguments follow [Evaluation order](#evaluation-order), like those of any helper.

C++ converts an operand narrower than `int` to `int` before an arithmetic or bitwise operator, so the result of `a + b` for two `int8` values is an `int`, which can be outside `int8`'s range. When the Go type of an operator expression in the table is `int8`, `int16`, `uint8`, or `uint16`, but not a defined type, whose operators convert their own results (see [Operators of defined types](#operators-of-defined-types)), convert its result back: `a + b` is `int8_t(a + b)`, `^b` is `uint8_t(~b)`, and `s[i] - '0'` is `uint8_t(eo::at(s, i) - '0')`. The conversion wraps modulo 2<sup>N</sup>, as Go's arithmetic does. A helper already returns the type of `x`, and a compound assignment, `++`, and `--` convert when they assign, so none of them needs a conversion. A comparison yields `bool` and needs none either.

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

Go evaluates a constant expression exactly and converts an untyped constant to a type only where it is used. C++ would evaluate the same expression in fixed-width types with C++ precedence: `1<<2 + 1` is 5 in Go and 8 in C++, and `x := 1 << 40` overflows a C++ `int`. So the translation writes a constant from its value, which the Go type checker records (`go/types` `TypeAndValue.Value`), rather than from its source. `T{L}` below is the form in [Untyped constant arguments](#untyped-constant-arguments): `T` is the translated type that the constant converts to, and `L` is a C++ literal of the folded value. Each rule below gives way to [Named constants](#named-constants): a declared constant used on its own is written by its name when that rule allows it, as in `x := KB`, which is `auto x = KB;`.

- A variable declared without a type, `x := c` or `var x = c`, is `auto x = T{L};`, where `T` is the type of a typed constant `c`, as in `x := int8(1)`, which is `auto x = int8_t{1};`, or the default type of an untyped one: `int64_t` for an integer constant, `int32_t` for a rune constant, and `double` for a floating-point one. `n := 10` is `auto n = int64_t{10};`, `r := 'a'` is `auto r = int32_t{97};`, and `f := 1.5` is `auto f = double{1.5};`. A boolean constant is `true` or `false`, and a string constant follows [Strings](#strings), so their C++ types are already Go's: `ok := true` is `auto ok = true;`.
- A numeric constant converted to an interface, such as an operand of `fmt.Println`, also takes its default type and is written `T{L}`, unless it is a declared constant written by name (see [Named constants](#named-constants)): `fmt.Println(1, 'a', 2.5)` is `fmt::Println(int64_t{1}, int32_t{97}, double{2.5})`.
- A constant that is the first operand of `eo::div`, `eo::rem`, `eo::shl`, or `eo::shr` is written `T{L}`, unless it is a declared constant written by name, because the helper takes its type from that operand: `1 << n` with an `int64` result is `eo::shl(int64_t{1}, n)`.
- Anywhere else, Go converts the constant to the type of the context, such as the other operand of an operator, a parameter, an assigned or declared variable, a result, or a slice element. The constant is written as in the source when it is a single literal, or a single literal after `-`, of one of these kinds:

  - an integer literal whose value is from -2147483647 to 2147483647, so that C++ reads it as an `int`, converted to an integer type or to `float64`: `i < 10`, `x * -2`, `New<int64_t>(0)`;
  - a rune literal whose value is below `0x80`, as an operand of an operator that the translation writes as a C++ operator, where C++ promotes the `char` to `int`: `b == 'a'`, `s[i] - '0'`. A rune literal anywhere else, such as an index or an argument, is written as its decimal value, because the helpers reject a `char`: `table['a']` is `eo::at(table, 97)`, and `x << 'a'` is `eo::shl(x, 97)`;
  - a floating-point literal converted to a float type: `f * 0.5`. A literal whose value rounds to zero in the target type drops a leading `-`, because a Go constant has no negative zero: `f * -0.0` is `f * 0.0`, and `-1e-400` converted to `float64` is `0.0`.

  `_` digit separators are removed, an octal prefix `0o` or `0O` is written `0`, and a floating-point literal converted to `float32` gets an `f` suffix, as in `0.25f`. The usual arithmetic conversions of C++ then give the operation Go's type, and a narrow result is converted back as described in [Arithmetic operators](#arithmetic-operators). Any other constant is written `T{L}`: `x + 1<<20` is `x + int64_t{1048576}`, `u & 0xffffffff` with a `uint32` `u` is `u & uint32_t{4294967295u}`, `n * 2.0` with an `int` `n` is `n * int64_t{2}`, and `[]float32{16777217}` is `slice<float>::of({float{1.6777216e+07f}})`. A constant converted to a [defined type](#defined-types) is always `T{L}`, as in `c > Celsius{30.0}` (see [Constants of defined types](#constants-of-defined-types)).

A constant expression that contains a declared constant, such as `KB * 3 / 2` after `const KB = 1024`, is written the same way, from its value: `int64_t{1536}`. A declared constant on its own is written by its name when the rule in [Named constants](#named-constants) allows it. The argument temporaries of `go` and `defer` statements are always `T{L}`, even for a single literal or a constant name (see [Untyped constant arguments](#untyped-constant-arguments)).

#### Constant declarations

Each constant that a `const` declaration declares is one C++ declaration, in source order. Its initializer is the constant's value, which the Go type checker records (`go/types` `Const.Val`), so `iota` and the repeated expressions of a constant group need no translation of their own. `T` is the constant's translated type, or the default type of an untyped constant, as above: `int64_t`, `int32_t`, or `double`. `L` is the literal described in [Untyped constant arguments](#untyped-constant-arguments).

| Constant | C++ declaration |
| --- | --- |
| integer, rune, or floating-point | `constexpr T N = L;` |
| boolean | `constexpr bool N = L;` |
| string | `const std::string N = L;` |

```go
const Greeting = "hello"
const (
	KB = 1 << (10 * (iota + 1))
	MB
)
const MaxU8 uint8 = 255
const Pi = 3.14159265358979323846264338327950288
const Big = 1 << 100
```

```cpp
const std::string Greeting = "hello"s;
constexpr int64_t KB = 1024;
constexpr int64_t MB = 1048576;
constexpr uint8_t MaxU8 = 255u;
constexpr double Pi = 0x1.921fb54442d18p+01;
```

- An untyped constant whose value its default type cannot represent, such as `Big`, or `1e400`, whose value overflows `float64`, has no declaration. Go uses it only inside constant expressions, which are written from their values, and every such expression is written without it.
- The blank identifier `_` declares nothing.
- A constant declared in a function is `static`: `const n = 10` is `static constexpr int64_t n = 10;`. A function literal or a goroutine then reads it without capturing it, and it does not end with the frame. A local constant is renamed as in [Redeclared names](#redeclared-names) when it is declared directly in the body of a statement whose clause declares the same name, as `const i = 10` in the body of `for i := 0; ...`, which is `static constexpr int64_t _eo_shadow_i = 10;`. The other case there does not arise, because the initializer is a literal. A `goto` that jumps over a local constant declaration follows [Goto](#goto).
- The package-level constants come first in a translated file, before the declarations listed in [Declaration order](#declaration-order).
- Go allows a constant that is never used. A constant whose C++ name the translation never writes, because it is unused or because every use is written from a value, gets `[[maybe_unused]]`, as in `[[maybe_unused]] static constexpr int64_t unused = 1;`, so that `-Wunused` stays quiet.

A constant of a defined type, such as `const Monday Weekday = 1`, follows [Constants of defined types](#constants-of-defined-types). Complex constants are not covered yet.

#### Named constants

A declared constant that is used on its own, not as part of a larger constant expression, is written by its C++ name when the constant has a C++ declaration and its type there is the type that the use converts it to, which the Go type checker records for the identifier (`go/types` `TypeAndValue.Type`). Parentheses around the name do not make a larger expression, and they are kept as [Arithmetic operators](#arithmetic-operators) says, so `n * (KB)` is `n * (KB)`. Otherwise the constant is written from its value as `T{L}`, like any other constant:

- A typed constant is used with its own type, so it is always written by name: `u == MaxU8` with a `uint8` `u`.
- An untyped constant is written by name where it takes its default type: `n * KB` and `case KB:` with an `int` `n`, `x := KB`, which is `auto x = KB;`, `fmt.Println(KB)`, and `r * Pi` with a `float64` `r`. With another type it is `T{L}`: `y + KB` with an `int32` `y` is `y + int32_t{1024}`, and `f * Pi` with a `float32` `f` is `f * float{0x1.921fb6p+01f}`. Go converts `Pi` to `float32` before it multiplies, while C++ would multiply `f * Pi` in `double`.
- A boolean or string constant is written by name, because its C++ type is already Go's, as in `if debug`, except where it converts to a defined type, where it is `T{L}`.
- The type checker records an untyped constant shift count as untyped. A count that is a declared constant of an integer type is written by name, and the rules for a constant count in [Arithmetic operators](#arithmetic-operators) apply to its value: `n << A` when `A` is less than the width of `n`'s type, and `eo::shl(n, A)` otherwise. Any other constant count, such as `const two = 2.0` or `1 << 63`, is written as its value as a decimal integer literal, with a `u` suffix when the value exceeds 9223372036854775807, and the same rules apply to that value: `n << 2`, and `eo::shl(n, 10000000000000000000u)` for `const big = 1e19`.

A constant name inside a struct definition or its methods follows [Package-level names hidden by members](#package-level-names-hidden-by-members), as `b.KB * ::KB` in a method of a struct with a field `KB`, and a constant hidden by a local declaration follows ordinary C++ scoping, as in Go.

#### Printing numbers

`fmt.Println` prints every integer type in decimal, including `int8_t` and `uint8_t`, which a C++ stream would print as characters. It prints a `float` or `double` as Go's `%v` does, which differs from fmt's `{}`: the shortest decimal that reads back as the same value, in exponent form when the decimal exponent is below -4 or at least 6, as in `1e+06` and `1.234567e+06`, and `+Inf`, `-Inf`, and `NaN`. Floats inside slices, maps, and structs print the same way.

A defined type with a numeric underlying type prints as its underlying value unless it has an `Error` or `String` method (see [Printing defined types](#printing-defined-types)).

Not covered yet: `complex64` and `complex128`, and the `math` package.

### Untyped constant arguments

Go evaluates an untyped constant expression exactly, in its own kind, and only then converts its value to the type the context requires. For a call argument, that type is the parameter type, or, for an interface parameter, the constant's default type (`bool`, `rune`, `int`, `float64`, `complex128`, or `string`), which is then converted to the interface.

This rule applies to the argument temporaries of `go` and `defer` statements, which are always written `T{L}`, even for a single literal or a constant name. Every other constant follows [Untyped constants](#untyped-constants).

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

Because of the `T{L}` form, the constant's C++ type is always `T`, even when `T` is a defined type whose literals would otherwise have the underlying type (see [Defined types](#defined-types)). Goroutine temporaries are declared with the parameter's value type, as for any argument. A defer temporary may stay `auto` when `T` is the parameter's value type; for an interface parameter, where `T` is the default type, declare the defer temporary with the parameter's value type, so that the conversion happens at the defer statement.

### Evaluation order

Go evaluates the function calls, method calls, receive operations, and binary logical operations of an expression in lexical left-to-right order. It does not specify when the other operands are read relative to them. C++ leaves the order of a function's arguments unspecified, and the operands of most C++ operators are unsequenced. The translation must not leave Go's order to the C++ compiler. The two orders can print different values, and they can also break memory safety. A reference that `eo::at` returns before a call that frees its backing array refers to freed storage. In `chs[0] <- grow()`, where `grow` appends to `chs`, the form `co_await (eo::at(chs, 0) << grow())` is a heap use after free under Clang with AddressSanitizer.

Eo fixes one order: an expression's nested ordered operations run first, in Go's order, and the operands that remain in the statement are read after them. The rules below say which operations are saved in temporaries to get that order.

#### Ordered operations

An ordered operation is one of these:

- a Go function or method call, including `co_await f(...)` of a coroutine, the call of a function value, and `std::apply(h, f())`, which is the call of `h`;
- a receive, `co_await *ch` or `co_await eo::recv_ok(ch)`;
- a `&&` or `||` whose operands contain an ordered operation.

Builtins, conversions, and `eo::` helpers are not ordered operations, but the ordered operations inside them are. This covers `len`, `cap`, `append`, `make`, `string(b)`, and `int64(x)`. A call inside a function literal's body belongs to that body. The operations inside an operand of `&&` or `||` belong to that operand (see [Short-circuit operands](#short-circuit-operands)).

#### Full expressions

The rule applies separately to each full expression that a statement evaluates:

- an expression statement;
- the initializer of a declaration;
- the operands of a `return`, together, as one expression with no outermost operation;
- the channel and value operands of a send statement, together. The send is the outermost operation, and it is not an ordered operation;
- both sides of an assignment or an op-assignment, together. When it translates to C++ `=` or a C++ compound assignment, which evaluates its right side first, the outermost operation of the right side is the outermost operation of the expression. When an op-assignment translates to an `eo::*_assign` helper, as `s[i] /= f()` does to `eo::div_assign(eo::at(s, i), f())`, the helper is the outermost operation, so every ordered operation on the right side is nested;
- the condition of an `if` or of a loop, and the tag of a `switch`;
- the case expressions of a `switch` clause, together, as the clause's condition (see [Case expressions that need temporaries](#case-expressions-that-need-temporaries));
- the range expression of a `for range` loop (see [Range loops](#range-loops));
- the case operands of a `select`, together, as one expression with no outermost operation. Their temporaries go before the `switch` statement of the [select shape](./SELECT.md#operand-evaluation);
- the function value, receiver, and arguments of a `go` or `defer` statement. Their saved operations come before the statement's function value, receiver, and argument temporaries.

An ordered operation in a full expression is nested unless it is the outermost operation. `fmt.Println` in `fmt.Println(f(), g())`, `h` in `s[g()] = h()`, and the receive in `v := <-ch` are not nested.

#### Saving ordered operations

Save every nested ordered operation of a full expression in a temporary, `auto _eo_call_<n> = ...;`, immediately before the statement, and use the temporary in its place, when any of these holds:

1. Two of its nested ordered operations are not inside one another, as `f()` and `g()` in `fmt.Println(f(), g())`. C++ may not run them in Go's order. For this condition, two operations in different elements of one braced list do not count, because C++ evaluates the elements left to right: `return {f(), g()};` and `Point{.X = f(), .Y = g()}` with its keys in declaration order need no temporaries. The list of `slice<T>::of` or `map<K, V>::of` is still subject to condition 2.
2. It contains a nested ordered operation and, anywhere in the full expression except inside that operation, one of these: a call to an `eo::` slice, string, or map helper; a call to `slice<T>::of` or `map<K, V>::of`; or a keyed struct literal whose keys are out of declaration order (see [Composite literals](#composite-literals)). A helper can return a reference that the operation invalidates, and such a literal evaluates its elements in a different order from Go. A helper that encloses the operation counts, so `x := s[g()]` saves `g()`, and so does `t := []int{f(), g()}`. A helper inside the operation runs before it, so `fmt.Println(errors.Join([]error{e}))` stays as written, while `fmt.Println(f(s[0], k()))` saves `k()` and then `f`, because `s[0]` is not inside `k()`.
3. It is an assignment whose left side contains an ordered operation and whose right side contains one, as in `*p() = g()`. C++ evaluates the right side of `=` and of a compound assignment before the left side. Go evaluates the operands on the left first.

Otherwise the full expression stays as written. A single nested ordered operation already runs before the outermost operation in C++. Go does not order it with the other operands. An operation inside another one runs first in C++ too, so `fmt.Println(f(g()))` stays as written.

```go
fmt.Println(f(), g())
x := f() + g()*h()
s[0] = s[1] + g()
s[g()] = h()
p().M(a(), b()) // func (*T) M(x, y int)
*p() = g()
chs[0] <- g()
fmt.Println(<-ch, g())
```

```cpp
auto _eo_call_0 = f();
auto _eo_call_1 = g();
fmt::Println(_eo_call_0, _eo_call_1);
auto _eo_call_2 = f();
auto _eo_call_3 = g();
auto _eo_call_4 = h();
auto x = _eo_call_2 + _eo_call_3 * _eo_call_4;
auto _eo_call_5 = g();
eo::at(s, 0) = eo::at(s, 1) + _eo_call_5;
auto _eo_call_6 = g();
eo::at(s, _eo_call_6) = h();
auto _eo_call_7 = p();
auto _eo_call_8 = a();
auto _eo_call_9 = b();
T::M(_eo_call_7, _eo_call_8, _eo_call_9);
auto _eo_call_10 = p();
*_eo_call_10 = g();
auto _eo_call_11 = g();
co_await (eo::at(chs, 0) << _eo_call_11);
auto _eo_call_12 = co_await *ch;
auto _eo_call_13 = g();
fmt::Println(_eo_call_12, _eo_call_13);
```

The details:

- The outermost operation stays in place, as `fmt.Println`, `M`, and `h` above. C++ evaluates the arguments and operands of an operation before it runs. When the outermost operation is a helper, an operator, a conversion, or a send, every ordered operation in the expression is nested, except inside an operand of `&&` or `||`. An assignment follows the rule in [Full expressions](#full-expressions): `s[i] = f()` and `s[i] += f()` keep `f()` in place, and `s[i] /= f()` becomes `auto _eo_call_0 = f();`, then `eo::div_assign(eo::at(s, i), _eo_call_0);`. `t := []int{f(), h(s[0], g())}` becomes `auto _eo_call_0 = f();`, `auto _eo_call_1 = g();`, `auto _eo_call_2 = h(eo::at(s, 0), _eo_call_1);`, then `auto t = slice<int64_t>::of({_eo_call_0, _eo_call_2});`. `s = append(s, g())` becomes `auto _eo_call_3 = g();`, then `s = eo::append(s, {_eo_call_3});`.
- Every nested operation is saved, including one inside another, which is saved first. This includes the arguments of a helper and the elements of a braced list, the receiver of a method call, the operand of a selector or a dereference, and the call that produces the function value of another call, as `next()` in `next()(s[0], g())`. `fmt.Println(f(s[0], k()), g())` becomes `auto _eo_call_0 = k();`, then `auto _eo_call_1 = f(eo::at(s, 0), _eo_call_0);`, then `auto _eo_call_2 = g();`. A method chain `p().Next(f()).Next(g())` saves `p()`, `f()`, the first `Next`, and `g()`, in that order.
- C++ evaluates the right side of an assignment before the left. In `s[g()] = h()` above, `h()` runs after the saved `g()` and before the index check, as Go orders them. In an op-assignment, `p().n += g()` becomes `auto _eo_call_0 = p();`, then `_eo_call_0->n += g();`.
- `<n>` counts the temporaries from 0 through the enclosing function, in the order they are declared, so no name is reused. That is statement by statement in source order and, within one statement, in Go's order of the operations. The bodies of function literals are numbered where they appear in the source.

#### Short-circuit operands

Go evaluates the right operand of `&&` and `||` only when the left one does not decide the result, so no operation is moved out of either operand. A nested `&&` or `||` that contains an ordered operation is saved as one temporary, in its place in Go's order: `fmt.Println(s[0], f() && h(), g())` becomes `auto _eo_call_0 = f() && h();`, then `auto _eo_call_1 = g();`. `i < len(t) && check(t[i])` needs no temporary.

Each operand is a full expression in its own right, whose outermost operation stays in place. The left operand always runs, so its temporaries go before the statement, as usual. When the right operand needs temporaries, the `&&` or `||` is computed by an `if` statement that assigns its temporary:

```go
ok := ready() && check(f(), g())
fmt.Println(done() || check(f(), g()), h())
```

```cpp
auto _eo_call_0 = ready();
if (_eo_call_0) {
  auto _eo_call_1 = f();
  auto _eo_call_2 = g();
  _eo_call_0 = check(_eo_call_1, _eo_call_2);
}
auto ok = _eo_call_0;
auto _eo_call_3 = done();
if (!_eo_call_3) {
  auto _eo_call_4 = f();
  auto _eo_call_5 = g();
  _eo_call_3 = check(_eo_call_4, _eo_call_5);
}
auto _eo_call_6 = h();
fmt::Println(_eo_call_3, _eo_call_6);
```

The temporary is initialized from the left operand, and it is declared where the `&&` or `||` would be saved. A `&&` assigns the right operand when the temporary is `true`, and a `||` when it is `false`. The right operand's temporaries are declared inside the `if`. The form applies when the `&&` or `||` is nested, and also when it is the whole full expression, as in `ok` above or the condition of an `if`, which then tests the temporary: `if (_eo_call_0) {`.

#### Positions that no statement can precede

A temporary must be declared before the expression that uses it. These positions have no place for a statement, so they take another form when they need temporaries. The init statement of a three-clause `for` loop is not one of them: its temporaries go immediately before the `for` statement.

- An `else if` condition: the `else` takes a block, which holds the condition's temporaries and then the `if` with the rest of the chain. `} else if f() + g() == 5 {` becomes `} else {`, the temporaries, `if (_eo_call_0 + _eo_call_1 == 5) {`, and one more closing brace after the chain.
- An `if` with an init statement, when the init statement or the condition needs temporaries: the statement becomes a block, `{`, that holds the init statement's temporaries, the init statement, the condition's temporaries, the `if` without an init statement, and `}`. The block gives the init statement's variables the same scope as a C++ init statement. A `switch` with an init statement is already a block (see [Switch statements](#switch-statements)).
- A loop condition: the condition moves into the body as `for (init;; post) {`, then the condition's temporaries, then `if (!(c)) { break; }` before the translated body, with `!c` for a condition `c` that is a single name. The condition still runs before every iteration, after the post statement. `continue` jumps to the post statement as before. A `for c {}` loop is `for (;;) {`. The post expression of [Loop clause variables](#loop-clause-variables) stays, so the condition reads each iteration's box.

```go
for i := 0; i < f()+g(); i++ {
	fmt.Println(i)
}
```

```cpp
for (auto i = int64_t{0};; i++) {
  auto _eo_call_0 = f();
  auto _eo_call_1 = g();
  if (!(i < _eo_call_0 + _eo_call_1)) {
    break;
  }
  fmt::Println(i);
}
```

#### Other operands

The gc compiler does not always follow Eo's order for operands that are not ordered operations. It reads `s`, `s[i]`, and a slice or string variable after the calls, as Eo does. It evaluates `len`, `cap`, slice expressions, and string indexing of a variable that are written before a call ahead of that call, so `fmt.Println(len(s), f(), g())`, where `g` assigns `s`, prints the old length. With one nested ordered operation and no helper, the C++ compiler may read such an operand before or after the operation too. The Go specification allows both orders, so a program whose output depends on this order is outside the mapping, and parity cases do not depend on it.

#### Evaluation order not covered yet

- a loop post statement that needs temporaries, which does not translate to a single C++ expression (see [Loop clause variables](#loop-clause-variables)).

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

`slice<T>{}` is the nil slice, because `T{}` is the zero value of every type in Eo. A composite literal is never nil, even when it is empty, so it is written `slice<T>::of({...})`. A nested literal whose element type Go elides spells it, and a `nil` element is `nullptr`: `[][]int{{1}, nil, {}}` is `slice<slice<int64_t>>::of({slice<int64_t>::of({1}), nullptr, slice<int64_t>::of({})})`. The braced list evaluates the elements left to right, and so does the braced list of `eo::append`, but the list is itself an argument of a function call, which C++ does not sequence against the other arguments. A Go call in a literal or in an append therefore follows [Evaluation order](#evaluation-order), like a call in any other operand of a helper.

Slicing a nil slice gives nil, and slicing any other slice gives a non-nil one, even when it is empty, as in Go. Appending nothing returns the slice unchanged, so `append(s)` and `append(s, t...)` with an empty `t` keep a nil `s` nil.

`eo::at`, `eo::subslice`, and `eo::make_slice` accept the same Go integer types as `eo::at` on a string (see [Strings](#strings)). An index out of range panics with Go's runtime error. A slice's high bound is checked against its capacity, so slicing a slice of capacity 3 as `s[:5]` panics with `runtime error: slice bounds out of range [:5] with capacity 3`, and the three-index form checks `max`, then `high`, then `low`, with Go's messages, such as `slice bounds out of range [:3:2]`. `make` panics with `runtime error: makeslice: len out of range` for a negative length, and `makeslice: cap out of range` for a capacity below the length. A length or capacity whose storage exceeds Go's 2<sup>48</sup> byte allocation limit panics in the same way, measured with the C++ size of `T`, which can differ from Go's.

`eo::at` returns a reference into the backing array, so `s[i] = v` is `eo::at(s, i) = v`, and `s[i] += v` is `eo::at(s, i) += v`. C++ evaluates the right operand of an assignment before the left, so `eo::at(s, i) = f()` calls `f` before it checks `i`, as the gc compiler does. When the left side also contains a call, as in `s[g()] = f()`, [Evaluation order](#evaluation-order) applies. Do not bind the reference to a C++ reference variable: an `append` past capacity or an assignment to `s` can leave it referring to an array that is no longer shared or no longer alive. Take the address with the pointer form, `ptr(s, &eo::at(s, i))`, instead (see [Pointer type](#pointer-type)).

`append` writes in place when the new elements fit in the capacity of `s`, so other slices of the same backing array observe them, as in Go. Otherwise it copies the elements into a new array, and the result no longer shares elements with `s`. The capacity of the new array follows the formula of Go's runtime, `nextslicecap`, without its rounding to allocation size classes. Go does not specify this capacity, and the gc compiler's choice depends on the element size and on escape analysis: a slice that does not escape may start in a 32-byte stack buffer. A program whose behavior depends on the capacity after a reallocating `append` is outside this mapping, and parity cases do not print it. `copy` and `eo::append_slice` handle overlapping elements as Go does, as if through a temporary buffer.

Pass, return, and store slices by value, as `slice<T>`, never as `const slice<T>&`, for the same reason as pointers. A slice variable is boxed by the rules in [Boxed variables](#boxed-variables), like any other variable. A function literal that captures a slice by value still shares its elements.

`fmt::Println` prints a slice as Go does: its elements in brackets separated by spaces, `[]` for nil, and a `[]byte` as numbers.

`begin()` and `end()` give the elements, so std algorithms work over a slice, as in `std::sort(s.begin(), s.end())`. They are not a translation of a Go form.

The slice helpers are function objects with the same collision rule as the string helpers (see [Strings](#strings)).

`for range` over a slice is in [Range loops](#range-loops), and keyed elements in a composite literal, such as `[]T{2: x}`, and slices of arrays are in [Arrays](#arrays). Not covered yet: the functions of the `slices` package that [The slices package](#the-slices-package) does not list ([#94](https://github.com/conr2d/eo/issues/94)); and variadic calls in `go` and `defer` statements ([#95](https://github.com/conr2d/eo/issues/95)).

`ptr(s, &eo::at(s, i))` names `s` twice, so `s` must be a variable, or a field or element path of one without calls; a slice operand with a call is saved first, by [Evaluation order](#evaluation-order), so that the pointer's owner and target are the same array.

#### The slices package

Go's `slices` package maps, in part, to the namespace `slices` from `<eo/slices.h>`, whose functions are called as Go calls them, like those of `maps` (see [Maps](#maps)): `slices.Clone(s)`, `slices.Delete(s, i, j)`, `slices.DeleteFunc(s, del)`, `slices.Index(s, v)`, `slices.IndexFunc(s, f)`, `slices.Contains(s, v)`, `slices.ContainsFunc(s, f)`, `slices.SortFunc(s, cmp)`, and `slices.SortStableFunc(s, cmp)` are `slices::Clone(s)`, `slices::Delete(s, i, j)`, and so on. A function literal argument is the lambda of [Function literals](#function-literals), passed directly, and a comparison function returns `int64_t`, as Go's returns `int`. Each function takes its slice as a `slice<E>`, and `slices::Clone`, `slices::Delete`, and `slices::DeleteFunc` return one, so a slice of a [defined type](#defined-types) `T` passes as it is, and a result that Go gives the type `T` is converted, as the result of `eo::append` is: `names = slices.Delete(names, 0, 1)` is `names = Names(slices::Delete(names, 0, 1));`.

The functions keep Go's aliasing and nil behavior. `slices::Clone` of nil is nil, and of an empty slice is an empty slice that is not nil. `slices::Delete` and `slices::DeleteFunc` move the kept elements within `s`, set the elements between the new length and the old one to the zero value, as Go does since Go 1.22, and return `s` shortened, so another slice of the same array sees both changes. `slices::Delete` panics as `s[i:j:len(s)]` does when its bounds are out of range, with the same runtime error. `slices::DeleteFunc` calls `del` once for each element, in order, and `slices::IndexFunc` and `slices::ContainsFunc` call `f` on the elements in order until it returns true.

`slices::SortFunc` sorts with `std::sort`, so equal elements can end in another order than Go's, which Go leaves unspecified too, and a program whose output depends on the order of equal elements is outside this mapping. `slices::SortStableFunc` keeps equal elements in their order, as Go does. `cmp` must be a strict weak order, as Go requires for a sorted result: Go sorts some order for any other `cmp`, and C++ gives no such promise.

### Arrays

Go `[N]T` maps to `std::array<T, N>`, which has Go's value semantics: assignment, arguments, and results copy the elements, and `==` compares them in order. The helpers for arrays come from `<eo/array.h>`, `<eo/slice.h>`, and `<eo/string.h>`, which `<eo/core.h>` includes.

| Go | C++ |
| --- | --- |
| `[N]T` | `std::array<T, N>` |
| the zero value of `[N]T`, `var a [N]T` | `std::array<T, N>{}`, `std::array<T, N> a{};` |
| `[N]T{a, b}`, `[...]T{a, b}` | `std::array<T, N>{a, b}`, `std::array<T, 2>{a, b}` |
| `[N]T{2: x, 4: y}`, `[...]T{1, 3: y}` | `eo::keyed_array<std::array<T, N>>({{2, x}, {4, y}})`, `eo::keyed_array<std::array<T, 4>>({{0, 1}, {3, y}})` |
| `[]T{2: x, y}` | `slice<T>::keyed(4, {{2, x}, {3, y}})` |
| `len(a)`, `cap(a)`, `len(p)`, `cap(p)`, where `p` is a pointer to an array | `eo::len(a)`, `eo::cap(a)`, `eo::len(p)`, `eo::cap(p)`, as `int64_t` |
| `a[i]`, `p[i]` | `eo::at(a, i)`, `eo::at(*p, i)` |
| `a == b`, `a != b` | `a == b`, `a != b` |
| `a[low:high]`, `a[low:]`, `a[low:high:max]`, where `a` is an array | `eo::subslice(pa, low, high)`, `eo::subslice(pa, low)`, `eo::subslice(pa, low, high, max)`, where `pa` is the translation of `&a` |
| `p[low:high]`, `p[low:]`, `p[low:high:max]` | `eo::subslice(p, low, high)`, `eo::subslice(p, low)`, `eo::subslice(p, low, high, max)` |
| `[N]T(s)`, where `s` is a slice | `eo::array_from_slice<std::array<T, N>>(s)` |

The C++ zero value must be written with `{}`: `std::array<int64_t, 4> a;` without it leaves the elements uninitialized. A literal that lists fewer elements than `N` fills the rest with zero values, as Go does. `[...]T` spells the length that Go counts. A nested literal whose element type Go elides spells it, as for slices: `[2]Point{{1, 2}, {3, 4}}` is `std::array<Point, 2>{Point{1, 2}, Point{3, 4}}`.

A literal with a keyed element gives every element its index: a positional element follows the previous one, and the first is at 0. `eo::keyed_array` and `slice<T>::keyed` start from zero values and store each value at its index. The braced list evaluates the elements in source order, as Go does. The length of `slice<T>::keyed` is one more than the largest index. Go's keys are constants in range, so they are not checked.

`eo::at` checks the index as for a slice, with Go's runtime error, such as `runtime error: index out of range [4] with length 4`. It returns a reference to the element of an array that is a variable, so `a[i] = v` is `eo::at(a, i) = v`, and a copy of the element of an array that is not, such as a call result: `f()[i]` is `eo::at(f(), i)`. `p[i]` dereferences `p` first, so a nil `p` panics before the index is checked, as in Go. `len` and `cap` of a pointer to an array give the length without reading the pointer, even a nil one, as Go's constant `len` does. Go does not evaluate `x` in `len(x)` and `cap(x)` when `x` is an array or a pointer to an array and contains no channel receive or non-constant function call, so such an `x` that is not a variable is not evaluated in C++ either: `len(*p)` is `eo::len(p)`, which a nil `p` also gives, and `len(s[i])` of a slice `s` of arrays is the constant, such as `int64_t{4}`, which checks no index.

A slice expression on an array takes its address, as Go defines `a[low:high]` as `(&a)[low:high]`. `pa` is therefore the pointer form of `&a` in [Pointer type](#pointer-type): `a` itself when `a` is [boxed](#boxed-variables), `ref(a)` when it is not, and `ref(x.f)` or `ptr(x, &x->f)` for an array field `x.f`, so `copy(h[:], b)` of a local `h` is `eo::copy(eo::subslice(ref(h), 0), b)`. [Boxed variables](#boxed-variables) counts the slice expression as taking the address of `a`, so a slice that can outlive `a` comes from a box and keeps the array alive. The result shares the array's elements, has the capacity `N - low`, or `max - low`, and checks its bounds against the array's length, with Go's messages, such as `runtime error: slice bounds out of range [:6] with length 5` and `slice bounds out of range [::7] with length 5`. A nil `p` panics with Go's nil dereference error before the bounds are checked. A slice of an empty array is empty and not nil.

`eo::array_from_slice` copies the first `N` elements of `s`. When `s` has fewer, it panics with Go's runtime error, `runtime error: cannot convert slice with length 2 to array or pointer to array with length 4`. A nil slice converts to an empty array only when `N` is 0.

An array is a map key when its element type is one (see [Maps](#maps)), and `fmt` prints the keys of such a map in Go's order, element by element. An array is comparable in an interface value when its element type is. `fmt` prints an array as it prints a slice, with its elements in brackets, so `[0]int{}` prints `[]`, and `%s`, `%q`, `%x`, and `%X` print a `[N]byte` as bytes, as for a `[]byte`. A pointer operand to an array prints as `&` followed by the array, and `%T` names an array type as Go does, as in `[4]uint8`. Range over an array and over a pointer to an array is in [Range loops](#range-loops).

Not covered yet: converting a slice to a pointer to an array, `(*[N]T)(s)` ([#131](https://github.com/conr2d/eo/issues/131)).

### Maps

Go `map[K]V` maps to `map<K, V>` from `<eo/map.h>`, which `<eo/core.h>` includes. A `map<K, V>` is a handle to a hash table: copying it copies the handle, and every copy, including a function's parameter, refers to the same entries, as in Go. The table stays alive for as long as any handle refers to it.

| Go | C++ |
| --- | --- |
| `map[K]V` | `map<K, V>` |
| `nil` as a `map[K]V`, and the zero value of `map[K]V` | `map<K, V>{}`, or `nullptr` where it converts; `var m map[K]V` is `map<K, V> m;` |
| `m == nil`, `m != nil` | `m == nullptr`, `m != nullptr` |
| `map[K]V{k1: v1, k2: v2}`, `map[K]V{}` | `map<K, V>::of({{k1, v1}, {k2, v2}})`, `map<K, V>::of({})` |
| `make(map[K]V)`, `make(map[K]V, n)` | `eo::make_map<K, V>()`, `eo::make_map<K, V>(n)` |
| `len(m)` | `eo::len(m)`, as `int64_t` |
| `m[k]` as a value | `eo::map_get(m, k)`, a copy of the value |
| `v, ok := m[k]` | `auto [v, ok] = eo::map_get_ok(m, k);` |
| `v, ok = m[k]` | `std::tie(v, ok) = eo::map_get_ok(m, k);` |
| `m[k] = v`, `m[k] += v`, `m[k]++` | `eo::map_entry(m, k) = v`, `eo::map_entry(m, k) += v`, `eo::map_entry(m, k)++` |
| `delete(m, k)` | `eo::delete_(m, k)` |
| `clear(m)` | `eo::clear(m)` |

The helpers are function objects, called qualified, with the same collision rule as the string helpers (see [Strings](#strings)). Go code rarely spells a name with an underscore, so the `map_` prefix keeps them apart from common Go names such as `lookup` and `entry`. `delete` is a C++ keyword, so the builtin is `eo::delete_`. A key argument converts to `K`, so a constant key is written as for any argument, and a rune literal key as its decimal value: `counts['z']++` is `eo::map_entry(counts, 122)++`.

`map<K, V>{}` is the nil map, which reads as empty: `eo::map_get` returns the zero value, `eo::map_get_ok` returns the zero value and `false`, `eo::len` returns 0, `eo::delete_` and `eo::clear` do nothing, and a `range` loop has no iterations. A composite literal is never nil, even when it is empty, so it is written `map<K, V>::of({...})`. The braced list evaluates its entries left to right, each key before its value, and a later entry replaces an earlier one with an equal key, which Go allows when the keys are not constants. A nested literal whose key or value type Go elides spells it: `map[string]Point{"a": {1, 2}}` is `map<std::string, Point>::of({{"a"s, Point{1, 2}}})`. The size argument of `eo::make_map` accepts any Go integer type and only sizes the table. Go rejects a negative constant size and treats a negative variable as zero, and so does Eo.

`eo::map_get` reads without inserting, so reading a missing key leaves `len(m)` unchanged, as in Go, while C++'s `operator[]` would insert it. `m[k].f` is `eo::map_get(m, k).f`. `eo::map_get_ok` returns a `std::pair` of the value and whether the key is present, and follows the forms of [Multiple results](#multiple-results) with a `std::pair`, as `eo::type_assert_ok` does: `_, ok := m[k]` is `auto ok = eo::map_get_ok(m, k).second;`.

`eo::map_entry` is for a map index on the left of an assignment, an op-assignment, `++`, or `--`. It returns a reference to the value in the table, inserting the key with the zero value first when it is absent, so `m[k] += 1` on a missing key stores 1. Assigning to an entry of a nil map panics with Go's `assignment to entry in nil map`, whose value is a `runtime.plainError`. An op-assignment that needs a helper for its operator takes the entry as its operand: `m[k] /= d` is `eo::div_assign(eo::map_entry(m, k), d)`. C++ evaluates the right side of an assignment before the left, so `m[k] = len(m)` reads the length before the entry is inserted, as Go does. A map inside another map is indexed as a value first: `m[a][b] = v` is `eo::map_entry(eo::map_get(m, a), b) = v`. A call in a map operand follows [Evaluation order](#evaluation-order), like a call in an operand of any other helper.

A key type must be a bool, an integer or float type, a string, a pointer, `eo::empty_struct`, or an array or struct whose elements or fields are such types. Other key types do not compile. Keys compare as Go compares them: pointers by address, arrays element by element, and structs field by field. A float key follows Go too. `-0.0` and `0.0` are the same key, and assigning with one of them stores it as the key, so the map prints the sign of the last assignment. Every assignment with a NaN key adds a new entry, which no lookup, assignment, or `delete` can reach, and which only `clear` removes.

A map compares only with `nil`. A struct with a map field is not comparable, and comparing two interface values that hold maps panics with `runtime error: comparing uncomparable type map[string]int`, as in Go. Concurrent writes to a map, or a write concurrent with a read, are a data race in Eo, as for any other variable, and a program that has one is outside the mapping. Go detects some of them with the fatal error `concurrent map writes`, and Eo does not.

`fmt` prints a map as Go does, as `map[k1:v1 k2:v2]` with the keys in the order of Go's `internal/fmtsort`: numbers by value, with NaN first, strings by bytes, `false` before `true`, structs field by field, and pointers by address. A verb applies to each key and value, so `%d` of a `map[string]int` prints `map[%!d(string=k):7]`, as in Go. `%p` of a map prints its address, as for a pointer. An operand that is a pointer to a map prints as `&` followed by the map. `%T` prints the Go type, such as `map[string][]int`. Pointer keys print in address order, which differs from run to run, so parity cases do not print a map with pointer keys.

`for range` over a map is in [Range loops](#range-loops).

Go's `maps` package maps to the namespace `maps` from `<eo/maps.h>`, whose functions are called as Go calls them, like those of `errors` (see [Errors](#errors)): `maps.Clone(m)`, `maps.Copy(dst, src)`, `maps.DeleteFunc(m, del)`, `maps.Equal(m1, m2)`, and `maps.EqualFunc(m1, m2, eq)` are `maps::Clone(m)`, `maps::Copy(dst, src)`, `maps::DeleteFunc(m, del)`, `maps::Equal(m1, m2)`, and `maps::EqualFunc(m1, m2, eq)`. A function literal argument is the lambda of [Function literals](#function-literals), passed directly. `maps::Clone` of nil is nil, and `maps::Copy` into a nil map panics when `src` has an entry.

#### Maps not covered yet

- interface and channel key types;
- the `maps` functions that take or return an iterator, `All`, `Keys`, `Values`, `Insert`, and `Collect`, which wait on range over functions ([#98](https://github.com/conr2d/eo/issues/98)).

### Structs

A Go struct type maps to a C++ aggregate of the same name. A package-level struct first declares its Go type name, `_eo_name` (see [Method tables](#method-tables)). Its fields keep their names, order, and grouping, each with the translated type and the initializer `{}`, so a struct's fields start as their zero values and `T{}` is the zero value of `T`. A struct with a tagged or an embedded field then declares its field table, `_eo_fields` (see [Field tags and embedding](#field-tags-and-embedding)). After the fields, every struct declares a defaulted `operator==`, then its methods as static member functions, in the order Go declares them (see [Methods](#methods)), then the forwarders of its promoted methods and the table of its methods, `_eo_methods`, if it has methods (see [Method tables](#method-tables)):

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
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Labeled {
  static constexpr std::string_view _eo_name = "main.Labeled";

  std::string Name{};
  Point At{};
  slice<std::string> Tags{};
  ptr<Labeled> Next{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"name\""}, {}, {}, {}};

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

C++ evaluates the initializers of a braced list from left to right, so a literal whose keys are in declaration order makes its own calls in Go's order. Their order relative to the other arguments of an enclosing call follows [Evaluation order](#evaluation-order), as for any argument. A keyed literal with keys out of declaration order would make its own calls in another order. An expression that contains such a literal follows rule 2 of [Saving ordered operations](#saving-ordered-operations): each nested ordered operation in the expression is saved in an `_eo_call_<n>` temporary, in Go's order, before the statement:

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

Go resolves the package-level types of a file in any order, but C++ needs a struct to be complete before another struct holds it by value. A translated program of one file therefore has, after its package-level constants (see [Constant declarations](#constant-declarations)):

1. a forward declaration `struct T;` of every struct, interface, and [defined type](#defined-types) the file declares, in source order;
2. the struct, interface, and defined type definitions, in source order, except that a type that a struct holds by value, as a field or an embedded field, is defined first. Apply this recursively: before defining a struct, define each struct, interface, or defined type it holds by value that is not defined yet, in the order of its fields. A pointer or slice field needs only the forward declaration, and so does a type in an interface's method signatures. A defined type holds what a field of its underlying type would hold;
3. the constants of defined types, in source order (see [Constants of defined types](#constants-of-defined-types));
4. the definitions of the interface methods and of the forwarders of promoted methods (see [Interfaces](#interfaces)), in the order of their types' definitions, and for each type in the order it declares them;
5. a prototype of each function that is called above its definition, in source order;
6. the package-level variables, in source order (see [Declaring package-level variables](#declaring-package-level-variables));
7. the remaining declarations in source order, including the out-of-class definitions of methods where Go declares the methods, and the `init` functions;
8. `eo_init`, if the file defines it (see [Package initialization](#package-initialization)).

A package of a program of several files puts these declarations in its package header and its files, as [Package files](#package-files) says.

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

#### Field tags and embedding

`reflect` reports a field's tag and whether the field is embedded, which the C++ type does not record. A struct that has a field with a tag or an embedded field lists every field in its field table, after the fields: `static constexpr eo::field _eo_fields[] = {...};`, from `<eo/struct.h>`, with one entry per field in declaration order. The entry of a tagged field is `{.tag = "..."}`, with the tag's value as a C++ string literal, the entry of an embedded field is `{.embedded = true}`, and the entry of a field that is neither is `{}`. A struct without tags or embedded fields has no table, and `reflect` reports its fields as untagged and not embedded. `eo::field` is always qualified, because a Go program can name a field `field`.

```go
type Item struct {
	Base
	*Counter
	Name string `json:"name,omitempty"`
	Size int
}
```

```cpp
static constexpr eo::field _eo_fields[] = {{.embedded = true}, {.embedded = true}, {.tag = "json:\"name,omitempty\""}, {}};
```

A struct type declared inside a function cannot have static data members in C++, so it has no table, and `reflect` reports no tags and no embedded fields for it.

#### Printing

`fmt::Println` prints a struct as Go's `%v` does: its fields in braces, separated by spaces, with nested structs, slices, and maps printed the same way and a nil pointer as `<nil>`. A struct whose method set has an `Error` or `String` method prints by calling it (see [Printing methods](#printing-methods)). An operand that is a pointer to a struct, slice, or map prints as `&` followed by its target, as `&{1 2}`. A pointer inside another value prints as an address, which differs from Go's, so parity cases do not print one. The fields are visited with Boost.PFR, which needs a struct to be an aggregate without base classes, as every translated struct is.

#### Struct forms not covered yet

- printing a struct with a field whose name starts with an upper-case letter that is not ASCII, which Go exports and Eo does not (see [Printing methods](#printing-methods));
- anonymous struct types other than `struct{}`, conversions between struct types, and blank fields `_`;
- a method named after its own struct, which C++ does not allow, and a local struct type hidden by a member or a local variable where the translation spells it (see [Package-level names hidden by members](#package-level-names-hidden-by-members));
- generic struct types, such as `type List[T any] struct`, and type aliases, such as `type A = B`.

Method values and method expressions, including promoted ones, are described in [Function values](#function-values).

### Defined types

A Go defined type whose underlying type is not a struct or an interface, such as `type Celsius float64`, maps to a C++ struct of the same name derived from `eo::defined<T, U>`, from `<eo/defined.h>`, which `<eo/core.h>` includes. `U` is the translated underlying type: a [basic type](#basic-types), `std::string`, a `std::array`, a `slice`, a `map`, or an `fn`. The underlying type of `type Kelvin Celsius` is `float64`, so `Kelvin` derives from `eo::defined<Kelvin, double>` and does not get the methods of `Celsius`, as in Go. The struct first inherits the constructors of its base with `using defined::defined;`, then declares `_eo_name`, its methods as static member functions, and `_eo_methods` if it has methods, as a struct does (see [Methods](#methods) and [Method tables](#method-tables)). It has no fields and no `operator==` of its own:

```go
type Celsius float64

func (c Celsius) String() string {
	return fmt.Sprintf("%gC", float64(c))
}

type Counter uint32

func (c *Counter) Inc() {
	*c++
}

type Data []byte
```

```cpp
struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";

  static auto String(Celsius c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Celsius::String, "String">>;
};

struct Counter : eo::defined<Counter, uint32_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Counter";

  static void Inc(ptr<Counter> c);

  using _eo_methods = eo::methods<eo::method<&Counter::Inc, "Inc">>;
};

struct Data : eo::defined<Data, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Data";
};

auto Celsius::String(Celsius c) -> std::string {
  return fmt::Sprintf("%gC"s, double(c));
}

void Counter::Inc(ptr<Counter> c) {
  (*c)++;
}
```

A defined type is thus a C++ type of its own, so `fmt`, interfaces, type assertions, and type switches tell it apart from its underlying type, as Go does, and `%T` prints `main.Celsius`. Methods, method values, and method expressions are written as for a struct: `c.String()` is `Celsius::String(c)`, `n.Inc()` is `Counter::Inc(ref(n))` by the pointer rules, and `Weekday.Next` is `Weekday::Next`.

| Go | C++ |
| --- | --- |
| `var x T`, the zero value of `T` | `T x{};`, `T{}`, or `auto x = New<T>();` when `x` is [boxed](#boxed-variables) |
| a constant of type `T` | `T{L}` (see [Constants of defined types](#constants-of-defined-types)) |
| `T(x)` and `V(x)` between numeric types, where `T` is a defined type and `V` a basic one or another defined type | `T(x)` and `V(x)`, as in `Celsius(f)`, `double(c)`, and `int64_t(d)` |
| `T(x)` where `x` has `T`'s underlying string, array, slice, map, or function type, or is of another defined type with that underlying type, and `U(x)` | `T(x)` and `U(x)`, as in `Name(s)`, `std::string(n)`, and `std::array<uint8_t, 32>(h)` |
| another conversion to `T`, such as `Name(b)` with a `[]byte` `b` | `T(e)`, where `e` is the conversion to `U`: `Name(eo::string_from_bytes(b))` |
| a composite literal `T{...}` of a slice or map type | `T(U::of({...}))`, as in `Data(slice<uint8_t>::of({1, 2}))` |
| a composite literal `T{...}` of an array type | `T(e)`, where `e` is the literal of `U` (see [Arrays](#arrays)), as in `Hash(std::array<uint8_t, 32>{1, 2})` |
| `T(s)`, where `T` is an array type and `s` a slice | `eo::array_from_slice<T>(s)` |

#### Operators of defined types

When `U` is a number or `bool`, `eo::defined` holds the value and defines Go's operators on two values of `T`, each returning `T`, as Go's operators keep the defined type: the arithmetic and bitwise operators, `&^` as `x & ~y`, comparisons, compound assignments, `++`, `--`, and `!` of a boolean `T`. Each converts its result back to `U`, so a narrow `U` wraps as in Go, and a defined type needs none of the narrow-type conversions of [Arithmetic operators](#arithmetic-operators): `p + q` of a `type Prio int8` is `p + q`. Integer division, remainder, and shifts use the helpers under the same conditions as for a basic type, and the helpers take a defined integer type and return it: `p / q` is `eo::div(p, q)`, `n << k` is `eo::shl(n, k)`, `p >> 1` is `p >> 1`, and `x / 2` is `x / Weekday{2}`. Every other integer that a helper takes, such as an index, a slice bound, a `make` size, a shift count, or the operand of `eo::string_from_rune`, must have a basic type, so an operand of a defined integer type there is converted to its underlying type: `s[d]` is `eo::at(s, int64_t(d))`, and `string(d)` is `eo::string_from_rune(int64_t(d))`. `a && b` and `a || b` of a boolean `T` yield `bool`, so they are written `T(a && b)` and `T(a || b)` except as a condition.

When `U` is a string, array, slice, map, or function type, `eo::defined` derives from `U`, so the helpers, operators, and members of `U` take a `T` as they take a `U`: `eo::len(d)`, `eo::at(d, i)`, `eo::map_get(s, k)`, `eo::map_entry(s, k) = v`, `eo::range(d)`, `d == nullptr`, `f(x)`, `h == g` of arrays, and `n < m` and `n += m` of strings. A slice expression on a `T` array takes its address as for any array, so `h[:]` of a `ptr<Hash> h` is `eo::subslice(h, 0)`, and its result is a slice of the element type, as in Go. A value of an unnamed array, slice, map, or function type `U` converts to `T` implicitly, as Go assigns a value of an unnamed type to a defined type with the same underlying type: `Op op = [=](int64_t x) mutable -> int64_t { return x * 3; };`. A `std::string` and a value of another defined type convert only explicitly, `Name(s)`, as Go requires. `n + m` of a string `T` returns `T`. Every helper whose result has type `U` in C++ and `T` in Go, such as `eo::append`, `eo::subslice`, and `eo::substr`, is converted: `d = append(d, 10)` is `d = Data(eo::append(d, {10}));`, and `n[1:]` is `Name(eo::substr(n, 1))`.

#### Constants of defined types

A constant of a defined type is declared as in [Constant declarations](#constant-declarations), with `T{L}` as its value: `constexpr T N = T{L};`, or `const T N = T{L};` when `U` is `std::string`. These declarations follow the type definitions (see [Declaration order](#declaration-order)), because C++ needs `T` complete. A constant converted to a defined type is always written `T{L}`, including a single literal and a boolean or string constant, because the operators of `T` take two values of `T`. A declared constant of type `T` used on its own is written by name, as any typed constant (see [Named constants](#named-constants)):

```go
type Weekday int

const (
	Sunday Weekday = iota
	Monday
)

const Default Name = "anon"

next := (d + 1) % 7
ok := d == Monday && c > 30
```

```cpp
constexpr Weekday Sunday = Weekday{0};
constexpr Weekday Monday = Weekday{1};

const Name Default = Name{"anon"s};

auto next = (d + Weekday{1}) % Weekday{7};
auto ok = d == Monday && c > Celsius{30.0};
```

#### Printing defined types

`fmt` prints a value of a defined type by its `Error` or `String` method, as described in [Printing methods](#printing-methods), and otherwise as its underlying value: a `Celsius` without methods prints as `37.5`, and a `Data` as `[1 2]`. A verb that the underlying type does not take prints the bad-verb form with the name of the defined type, as `%!s(main.Flag=true)`. `%d` of a `Weekday` with a `String` method prints its number, as in Go. A pointer operand to a defined array, slice, or map type prints as `&` followed by its target. A defined type whose underlying type can be a map key can be a key itself, and fmt sorts those keys by their underlying values.

#### Defined types not covered yet

- defined pointer and channel types, defined types whose underlying type is a struct, such as `type B A` for a struct `A`, and generic defined types;
- defined types declared inside a function;
- `range` over a value of a defined integer type.

### Switch statements

An expression `switch` maps to an `if` statement whose init statement saves the tag in `_eo_switch`, followed by an `else if` for each case clause in source order and an `else` for `default`, wherever Go writes it, as [Type switches](#type-switches) do. A clause's condition compares the tag with its case expression, `_eo_switch == e`, and a clause with several expressions joins their comparisons with `||` in order. The comparison is translated as any other `==`, so `case nil:` is `_eo_switch == nullptr`, and a value compared with an interface tag is converted, `_eo_switch == I(v)` (see [Interface values](#interface-values)). A `switch` without a tag compares with `true`, so a clause's condition is its case expression, or the expressions joined by `||`, and there is no `_eo_switch`. A clause with an empty body has an empty block.

```go
switch s {
case "a", "b":
	fmt.Println("a or b")
default:
	fmt.Println("other")
case "c":
	fmt.Println("c")
}
switch {
case err == nil:
case errors.Is(err, ErrMissing):
	return 0
}
```

```cpp
if (auto _eo_switch = s; _eo_switch == "a"s || _eo_switch == "b"s) {
  fmt::Println("a or b"s);
} else if (_eo_switch == "c"s) {
  fmt::Println("c"s);
} else {
  fmt::Println("other"s);
}
if (err == nullptr) {
} else if (errors::Is(err, ErrMissing)) {
  return 0;
}
```

Go evaluates the tag once, then the case expressions from top to bottom and left to right until one equals the tag, and runs that clause, or the `default` clause when none does. The `if` chain does the same: `||` and `else if` evaluate an expression only when the ones before it do not match, and a `default` clause that is not last in Go is still the final `else`, which does not change which clause runs. `_eo_switch` is a copy of the tag, so a case expression that assigns the tag's variable does not change the value that the later clauses compare with, as in Go. A tag that is an untyped constant is declared as `x := c` would be (see [Untyped constants](#untyped-constants)).

The other forms:

- A `switch` with an init statement puts it in a block of its own, `{ init; if ... }`, as a type switch does. The init statement's temporaries go in the block before it, and the tag's after it. The block gives the init statement's variables the scope that Go gives them, and the clauses' blocks are nested in it, so a clause can declare a variable of the same name without the renaming of [Redeclared names](#redeclared-names). A declaration that refers to the hidden variable, as `y := y * 10`, is still renamed.
- A `switch` whose only clause is `default`, or that has no clause, evaluates the tag as an expression statement, then runs the `default` body in a block of its own, as every clause has one, all in a block: `{ init; tag; { body } }`. A `switch` with no clause has no body block: `{ init; tag; }`. A `switch` without a tag and without a case clause is `{ init; { body } }`, or `{ body }` without an init statement. These forms have no `if` chain, and the label of a `break` (below) follows the body's block.
- An unlabeled `break` that leaves the `switch` becomes `goto _eo_break_<n>;`, and the label `_eo_break_<n>:;` follows the `if` chain, inside the block when the `switch` has one. This applies to a `break` in a clause that is not inside a nested `for`, `switch`, type switch, or `select`, and also to such a `break` in a type switch clause. `<n>` counts, from 0 in each function, the expression switches and type switches that have such a `break`, together, in the source order of their `switch` keywords, so an outer switch has a smaller number than a switch nested in one of its clauses. An unlabeled `continue` stays as written: it continues the enclosing loop in Go and in C++. A labeled `break` or `continue` follows [Labeled statements](#labeled-statements).
- `fallthrough` becomes `goto _eo_fallthrough_<n>;`, and the body of the next clause in source order starts with the label `_eo_fallthrough_<n>:;`. `<n>` counts the `fallthrough` statements of each function from 0, in source order. The jump enters the next clause's block without evaluating its case expressions, as Go does. It also works when the next clause is a `default` that the chain moved to the end, or when `default` falls through to the clause after it, which the chain placed before it.

```go
for _, x := range xs {
	switch {
	case x < 0:
		if x < -5 {
			break
		}
		fmt.Println("small negative")
		fallthrough
	default:
		fmt.Println("checked", x)
	}
}
```

```cpp
for (auto [_eo_range_key, x] : eo::range(xs)) {
  if (x < 0) {
    if (x < -5) {
      goto _eo_break_0;
    }
    fmt::Println("small negative"s);
    goto _eo_fallthrough_0;
  } else {
  _eo_fallthrough_0:;
    fmt::Println("checked"s, x);
  }
_eo_break_0:;
}
```

#### Case expressions that need temporaries

The tag and the condition of each clause are full expressions (see [Evaluation order](#evaluation-order)). The tag's temporaries come before the `if` chain. A later clause's condition is in an `else if` position, so its temporaries follow [Positions that no statement can precede](#positions-that-no-statement-can-precede): the `else` takes a block that holds them. A condition with several case expressions is a `||`, so a case expression that needs temporaries after the first one follows [Short-circuit operands](#short-circuit-operands). The comparisons of a clause stay in place as one `||` chain, `_eo_switch == a || _eo_switch == b || _eo_switch == c`: the operations inside an operand of `||` belong to that operand, so an inner `||` is never saved as a temporary of the outer one. A `switch` without a tag has no tag to run first: the first clause's temporaries go immediately before the `if` chain, as for any `if`, inside the block of the init statement when there is one. In a `switch` with a tag, the first clause's temporaries must run after the tag, so the tag is then declared as a statement, `auto _eo_switch = tag;`, followed by the temporaries and the `if` chain, all in a block:

```go
switch trace(n) {
case f() + g():
	fmt.Println("first")
case h(), k() * m():
	fmt.Println("second")
}
```

```cpp
{
  auto _eo_switch = trace(n);
  auto _eo_call_0 = f();
  auto _eo_call_1 = g();
  if (_eo_switch == _eo_call_0 + _eo_call_1) {
    fmt::Println("first"s);
  } else {
    auto _eo_call_2 = _eo_switch == h();
    if (!_eo_call_2) {
      auto _eo_call_3 = k();
      auto _eo_call_4 = m();
      _eo_call_2 = _eo_switch == _eo_call_3 * _eo_call_4;
    }
    if (_eo_call_2) {
      fmt::Println("second"s);
    }
  }
}
```

#### Switch statements not covered yet

- a `fallthrough` whose target clause's block is inside an `else` block that declares temporaries, when the `fallthrough` itself is outside that `else` block. This happens when the target's condition, or for a `default` target any condition of the chain, needs temporaries and comes after the clause that falls through in the chain's order, which places `default` last. The jump would skip the temporaries' declarations, which C++ rejects.

### Range loops

A `for range` loop over an integer, a slice, a string, or a map maps to a C++ range-based `for` statement over `eo::range(x)`, from `<eo/range.h>`, which `<eo/core.h>` includes. C++ evaluates `eo::range(x)` once, before the first iteration, as Go evaluates the range expression, and the result keeps its own copy of `x`.

A loop over a channel cannot use a range-based `for`, and is in [Range over channels](#range-over-channels).

| Go | C++ |
| --- | --- |
| `for i := range n`, where `n` is an integer | `for (auto i : eo::range(n))` |
| `for i, v := range s`, where `s` is a slice | `for (auto [i, v] : eo::range(s))` |
| `for _, v := range s` | `for (auto [_eo_range_key, v] : eo::range(s))` |
| `for i := range s` | `for (auto i : eo::range(eo::len(s)))` |
| `for i, r := range str`, where `str` is a string | `for (auto [i, r] : eo::range(str))` |
| `for _, r := range str` | `for (auto [_eo_range_key, r] : eo::range(str))` |
| `for i := range str` | `for (auto [i, _eo_range_value] : eo::range(str))` |
| `for i, v := range a`, where `a` is an array or a pointer to an array | `for (auto [i, v] : eo::range(a))` |
| `for i := range a` | `for (auto i : eo::range(eo::len(a)))` |
| `for k, v := range m`, where `m` is a map | `for (auto [k, v] : eo::range(m))` |
| `for _, v := range m` | `for (auto [_eo_range_key, v] : eo::range(m))` |
| `for k := range m` | `for (auto [k, _eo_range_value] : eo::range(m))` |
| `for range x` | `for ([[maybe_unused]] auto _eo_range_key : eo::range(x))`, with `eo::len(s)` in place of a slice, an array, or a pointer to an array `s` |

A blank variable is the same as an omitted one, so `for i, _ := range s` is `for i := range s`, and `for _ = range x` and `for _, _ = range x` are `for range x`. Where the clause needs a C++ name that the Go source does not give, it uses the hidden name of the position: `_eo_range_key` for the first variable and `_eo_range_value` for the second. A clause never declares the same hidden name twice.

- An integer `n` may have any Go integer type, and `i` has the same type. It takes the values 0 through `n-1`, and none when `n` is 0 or negative. An untyped constant `n` that is not a declared constant written by name (see [Named constants](#named-constants)) is written `T{L}`, as in [Untyped constant arguments](#untyped-constant-arguments), where `T` is the constant's default type, or the type of the variable in the assignment form below: `for i := range 3` is `for (auto i : eo::range(int64_t{3}))`, and `for i := range 'c' - 'a'` is `for (auto i : eo::range(int32_t{2}))`, because the default type of a rune constant is `rune`.
- For a slice, `i` is an `int64_t` and `v` is a copy of the element. `eo::range(s)` holds a copy of `s`, so the number of iterations is `len(s)` when the loop starts: assigning `s` or appending to it in the body does not change the loop. Each element is read from the original backing array when its iteration starts, so the loop sees what the body wrote to a later element, as in Go. `for i := range s` reads no element, as in Go, so it ranges over `eo::len(s)`.
- For an array, `i` is an `int64_t` and `v` is a copy of the element. Go evaluates the range expression once, before the loop, so `eo::range(a)` holds a copy of the array, and the body's writes to `a` do not change the values that the loop produces. For a pointer to an array, `eo::range(p)` holds a copy of `p` and reads each element through it when its iteration starts, so the loop sees what the body wrote to a later element, as in Go; a nil `p` panics when the first element is read, and an empty array reads none. `for i := range a` reads no element, and Go does not evaluate `a` when `len(a)` is a constant, so it ranges over the translation of `len(a)` in [Arrays](#arrays): `for i := range *p` ranges over `eo::len(p)`, and a nil `p` gives `N` iterations without a panic.
- For a map, `k` and `v` are copies of each key and its value, in an unspecified order. Go randomizes the order, so a program that depends on it is outside the mapping, and parity cases do not print in it. `eo::range(m)` holds a copy of `m` and of the keys that `m` has when the loop starts, so assigning `m` in the body does not change the loop. Each iteration looks its key up again: an entry that the body deletes or clears before the loop reaches it is not produced, and `v` is the value that the entry has when its iteration starts, as in Go. An entry that the body adds is not produced, which Go allows: the specification leaves it to the implementation whether such an entry is produced. An entry whose key is NaN is produced unless the body clears the map.
- For a string, `i` is the `int64_t` byte index of each UTF-8 sequence and `r` is its code point as an `int32_t`. A byte that does not start a valid sequence gives U+FFFD and advances one byte, as `[]rune(str)` does (see [Slices](#slices)). `eo::range(str)` holds a copy of `str`, so assigning `str` in the body does not change the loop.

C++ declares the variables of a range-based `for` again for each iteration, as Go 1.22 does, so the loop's own update is not a write to the previous iteration's variables. A function literal that captures a range variable by reference and is not boxed captures it with `&`, as in `[=, &v]`, which C++20 allows for a structured binding.

An unlabeled `break` and `continue` are C++ `break` and `continue`, and labeled ones follow [Labeled statements](#labeled-statements). The range expression is the argument of `eo::range` and follows [Evaluation order](#evaluation-order) as if it were the initializer of a declaration, so its outermost call stays inside `eo::range(...)`, as in `for (auto [_eo_range_key, v] : eo::range(next()))`. A nested operation that the rule saves goes before the `for` statement, as in `for _, v := range next()[g():]`, which is `auto _eo_call_0 = next();`, `auto _eo_call_1 = g();`, and `for (auto [_eo_range_key, v] : eo::range(eo::subslice(_eo_call_0, _eo_call_1)))`.

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

#### Range over channels

C++ cannot `co_await` in the header of a range-based `for`, so a `for range` loop over a channel is a `for (;;)` loop that saves the channel in its init statement and receives at the start of each iteration with `eo::recv_ok`. It stops when the channel is closed and drained:

```go
for v := range ch {
	fmt.Println(v)
}
```

```cpp
for (auto _eo_range_chan = ch;;) {
  auto [v, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
  if (!_eo_range_ok) {
    break;
  }
  fmt::Println(v);
}
```

| Go | First statement of the C++ body |
| --- | --- |
| `for v := range ch` | `auto [v, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);` |
| `for range ch`, `for _ = range ch` | `auto [_eo_range_key, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);` |
| `for v = range ch` | `auto [_eo_range_key, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);`, and `v = _eo_range_key;` after the `if` |

The `if (!_eo_range_ok) { break; }` statement always follows the receive. The init statement evaluates the range expression once, as Go does, so assigning `ch` in the body does not change the loop, and its outermost call stays in the init statement, as in `for (auto _eo_range_chan = produce(5);;)`. A nested Go call in it is saved before the `for` statement, as for other range expressions. `continue` jumps to the next receive and `break` leaves the loop, as in Go. A nil channel blocks the loop forever.

A boxed range variable is declared under its hidden name, `_eo_range_key`, and boxed right after the `if`, as in [Boxed range variables](#boxed-range-variables): `auto n = New<int64_t>(_eo_range_key);`. The structured binding is declared again in each iteration, so a goroutine or a pointer keeps the value of its own iteration. A variable that the body declares with the name `v` is renamed as in [Clause variables redeclared in the body](#clause-variables-redeclared-in-the-body). A loop that ranges over a channel is in a coroutine, because it receives.

Not covered yet: `range` over a function ([#98](https://github.com/conr2d/eo/issues/98)).

### Labeled statements

A Go label names a statement for `goto`, and a `for`, `switch`, type switch, or `select` statement for `break` and `continue`. C++ has no labeled `break` or `continue`, so all three jumps become a C++ `goto` to a label that the translation places. A Go label `L` gives up to three C++ labels, and each is written only when a jump to it is:

| Go | C++ jump | C++ label |
| --- | --- | --- |
| `goto L` | `goto _eo_label_L;` | `_eo_label_L:;` immediately before the labeled statement, and before the temporaries of that statement |
| `break L` | `goto _eo_break_L;` | `_eo_break_L:;` immediately after the labeled statement: after the loop, after the `if` chain of a `switch` or type switch, or after the C++ `switch` of a `select`, inside the block of the statement when its translation has one |
| `continue L` | `goto _eo_continue_L;` | `_eo_continue_L:;` as the last statement of the loop's C++ body, after a block that holds the rest of the body (below) |

The labeled statement itself is translated as usual. A `break L` or `continue L` is a `goto` even when an unlabeled `break` or `continue` would reach the same place, and a `break L` does not count toward the `<n>` of `_eo_break_<n>` in [Switch statements](#switch-statements). An unlabeled `break` and `continue` keep their own rules. A Go label never starts with a digit, so `_eo_break_L` is never an `_eo_break_<n>`, and the prefix also keeps a label such as `class`, which Go allows, from being a C++ keyword. Go and C++ both scope labels to a function, and a function literal, which is a C++ lambda, has its own. Go forbids a jump into a block, so every jump leaves blocks, including the body of a loop and the C++ `switch` of a `select`, and never enters one.

#### Labeled loops

When a loop is the target of a `continue L`, its translated body, everything that its C++ body would hold, goes in a block of its own, and `_eo_continue_L:;` follows the block. A variable declared directly in the Go loop body is still declared directly in a loop body for rule 2 of [Boxed variables](#boxed-variables), although this block holds it. The body includes the boxes of [Boxed range variables](#boxed-range-variables), the receive of [Range over channels](#range-over-channels), and the temporaries and the `if (!(c)) { break; }` of a [loop condition](#positions-that-no-statement-can-precede) that moved into the body. C++ rejects a `goto` that jumps forward over a declaration into its scope, and the body's declarations end with the block, so the jump skips none of them. After the label, the loop runs its post statement, including the new boxes of [Loop clause variables](#loop-clause-variables), as Go's `continue` does:

```go
rows:
	for r, row := range table {
		total := 0
		for _, v := range row {
			if v < 0 {
				continue rows
			}
			total += v
		}
		avg := total / len(row)
		fmt.Println(r, total, avg)
	}
```

```cpp
for (auto [r, row] : eo::range(table)) {
  {
    auto total = int64_t{0};
    for (auto [_eo_range_key, v] : eo::range(row)) {
      if (v < 0) {
        goto _eo_continue_rows;
      }
      total += v;
    }
    auto avg = eo::div(total, eo::len(row));
    fmt::Println(r, total, avg);
  }
_eo_continue_rows:;
}
```

A loop that is only the target of a `break L` keeps its body as it is, and `_eo_break_L:;` follows the loop. A `break L` or `continue L` in a clause of a `select` ends the clause's body without being a terminating statement, so the clause still ends with `break;` (see [Case body rule](./SELECT.md#case-body-rule)).

#### Goto

`goto L` jumps to `_eo_label_L:;`. A backward jump needs nothing more: C++ ends the scope of every variable declared after the label, and a declaration that runs again makes a new variable, so a boxed variable gets a new box each time, as in Go.

Go forbids a forward `goto` that brings a variable into scope, so no Go variable is declared between the jump and its label in the label's block. The translation adds declarations that Go does not have, and a forward jump must not skip them either. For each forward `goto`, consider the statements of the label's block after the one that contains the `goto` and before the labeled statement:

- A statement whose translation declares temporaries before it, such as the `_eo_call_<n>` of [Evaluation order](#evaluation-order), the argument temporaries of `go` and `defer`, and the temporaries of a loop's init statement, goes in a block of its own with them, `{ temporaries; statement }`. When that statement is itself labeled, its `_eo_label_L2:;` goes before the block, because a jump into a block is rejected. The temporaries of the labeled statement itself follow its label, so they need no block.
- A local constant is `static` (see [Constant declarations](#constant-declarations)), and C++ initializes a `static const std::string` only when control passes through its declaration. A local constant declared there, of any type, is therefore never written by name: every use is written from its value, and its declaration gets `[[maybe_unused]]`.

```go
	if n > 1 {
		goto done
	}
	fmt.Println("small", f()+f())
	const greeting = "hello"
	n = len(greeting)
done:
	return fmt.Sprint(n, " ", greeting)
```

```cpp
  if (n > 1) {
    goto _eo_label_done;
  }
  {
    auto _eo_call_0 = f();
    auto _eo_call_1 = f();
    fmt::Println("small"s, _eo_call_0 + _eo_call_1);
  }
  [[maybe_unused]] static const std::string greeting = "hello"s;
  n = 5;
_eo_label_done:;
  return fmt::Sprint(n, " "s, "hello"s);
```

C++ also rejects a jump into a `try` block. In a function with a `defer` statement, `try {` opens before the first top-level statement that contains a `defer` (see [Defer](#defer)). When a top-level statement before `try {` contains a `goto` whose label is inside the `try` block, `try {` opens instead before the first such statement, and before that statement's temporaries. Moving it can bring another label inside the `try` block, so the step repeats until no top-level statement before `try {` contains a `goto` to a label inside it. The locals declared after `try {` are then inside the `try` block, with what [Defer](#defer) and rule 2 of [Boxed variables](#boxed-variables) say about them. A panic before the first `defer` statement then reaches `eo_defer_panic` with no deferred call to run, which rethrows it unchanged.

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
| `&p[i]`, where `p` is a pointer to an array | `ptr(p, &eo::at(*p, i))` |
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

An unnamed or blank receiver, as in `func (Boom) String()`, is named `_eo_receiver`, and an unnamed or blank parameter of a method, a forwarder, or an interface method is named `_eo_arg_<n>`, `n` being its position among the parameters after the receiver, from 0.

A type with methods also lists them in its method table, `_eo_methods`, which interfaces and `fmt` use (see [Method tables](#method-tables)).

Methods of Eo library types, such as `sync::Mutex` and `sync::WaitGroup`, stay member functions, called as `x.M()` or `p->M()`.

#### Not covered yet

- printing a pointer with `fmt` inside another value, which shows an address;
- conversions through `unsafe.Pointer` or `uintptr`.

Method values and method expressions are described in [Function values](#function-values).

### Function values

A Go function type maps to `fn<Sig>` from `<eo/fn.h>`, which `<eo/core.h>` includes. `Sig` is the translated signature `R(A...)`, with `void` for no result and `std::tuple` for several (see [Multiple results](#multiple-results)): `func(int) string` is `fn<std::string(int64_t)>`, `func()` is `fn<void()>`, and `func(int, int) (int, int)` is `fn<std::tuple<int64_t, int64_t>(int64_t, int64_t)>`. A slot that can hold a coroutine has the signature `func<R>(A...)` instead, as described in [Coroutine function values](#coroutine-function-values).

| Go | C++ |
| --- | --- |
| `func(A...) R` | `fn<R(A...)>`, or `fn<func<R>(A...)>` for a coroutine slot |
| `nil` as a function value, and the zero value of a function type | `fn<Sig>{}`, or `nullptr` where it converts; `var f func()` is `fn<void()> f{};` |
| `f == nil`, `f != nil` | `f == nullptr`, `f != nullptr` |
| `f(args)`, where `f` is a function value | `f(args)`, or `co_await f(args)` for a coroutine slot |
| a function `g` used as a value | `g` |
| a function literal | the lambda described in [Function literals](#function-literals) |
| a method value `x.M` | a lambda, as described in [Method values](#method-values) |
| a method expression `T.M` or `(*T).M` | `T::M`, or a lambda, as described in [Method expressions](#method-expressions) |

A function, a lambda, or a plain `fn` converts to `fn<Sig>` implicitly, so it is written as is in an initializer, an assignment, an argument, a result, or an element of a braced list: `apply(twice, 4)`, `h.on = [=](std::string s) mutable -> std::string { return s + "!"s; };`, and `slice<fn<int64_t(int64_t)>>::of({twice, makeAdder(3), nullptr})`.

`x := v` is `auto x = v;` when `v` already has an `fn` type, such as the result of a call. Otherwise:

- `x := g` of a function or a method expression is `fn<Sig> x = g;`, so `x` is a function value and not a C++ function pointer;
- `x := v` of a function literal or a method value stays `auto x = v;`, as in [Function literals](#function-literals), when `x` is never assigned again, never compared with `nil`, and not [boxed](#boxed-variables). Otherwise it is `fn<Sig> x = v;`, because a lambda's type cannot hold another function or nil. A boxed function variable is `auto x = New<fn<Sig>>(v);`.

The behavior follows Go:

- Copies of an `fn` share its target, as copies of a Go function value share its closure. Copying an `fn` never copies a lambda's captured variables.
- A call keeps its target alive until it returns, also while a coroutine call is suspended. A function that assigns the variable holding it, as `self = nil` inside `self`, and another goroutine that replaces a field while a call through it is blocked, are safe.
- Calling a nil function value panics with `runtime error: invalid memory address or nil pointer dereference` after the arguments are evaluated, as in Go. A deferred `recover()` can stop it. For a coroutine slot, the panic comes from the call, before the `co_await` can suspend.
- An `fn` compares only with `nullptr`, as Go compares a function value only with nil. `fn` has no `==` of its own, so a struct with a field of function type has a deleted `operator==`, as described in [Structs](#structs).
- The call of a function value is an ordered operation (see [Evaluation order](#evaluation-order)), and so is a call that produces the function value: `fmt.Println(makeAdder(1)(2), g())` saves `makeAdder(1)`, then its call, then `g()`. C++ evaluates the function value of a call before its arguments, but `fn` reads its target when the call runs, after the arguments. In `f(g())`, where `g` assigns `f`, the call reaches the new value, as it does with gc. The Go specification does not order this read against the calls, so a program whose output depends on it is outside the mapping (see [Other operands](#other-operands)).

`defer f(args)` and `go f(args)` with `f` a variable of function type save `f` as a function-valued callee, `auto _eo_defer_fn_<n> = f;` and `auto _eo_go_fn_<n> = f;`, because Go evaluates the function value at the statement (see [Defer](#defer) and [Function-valued calls](./GOROUTINE.md#function-valued-calls)). A nil deferred function value panics when the deferred call runs, as in Go.

#### Method values

A method value `x.M` translates to a lambda whose init capture `_eo_receiver` holds the receiver that Go binds when it evaluates `x.M`. It calls the method in the [method form](#methods), with the parameters named `_eo_arg_<n>`:

| Go | C++ |
| --- | --- |
| `p.M`, pointer receiver, `p` a pointer | `[_eo_receiver = p](A _eo_arg_0) { return T::M(_eo_receiver, _eo_arg_0); }` |
| `x.M`, pointer receiver, `x` addressable | `[_eo_receiver = &x](A _eo_arg_0) { ... }`, with `&x` translated as in [Pointer type](#pointer-type), such as `ref(x)` |
| `x.M`, value receiver | `[_eo_receiver = x](A _eo_arg_0) { ... }`, with `x` the translated value of `x`, such as `*x` when `x` is boxed |
| `p.M`, value receiver, `p` a pointer | `[_eo_receiver = *p](A _eo_arg_0) { ... }` |

```go
add := c.Add // func (c *Counter) Add(d int) int
get := v.Get // func (c Counter) Get() int
```

```cpp
auto add = [_eo_receiver = c](int64_t _eo_arg_0) { return Counter::Add(_eo_receiver, _eo_arg_0); };
auto get = [_eo_receiver = v]() { return Counter::Get(_eo_receiver); };
```

The init capture runs where Go evaluates the method value. A value receiver is copied then, so a later change to `v` does not reach `get`, and `*p` on a nil `p` panics then, before any call. A pointer receiver keeps the pointer, and a nil one panics only where the method dereferences it, as in Go. The lambda is not `mutable`, because it never changes its capture, and its result type is the method's. A coroutine method gives a lambda that returns its `func<R>`, called as `co_await add(1)`. A promoted method binds the path to its embedded field, as in [Embedded fields](#embedded-fields).

#### Method expressions

| Go | C++ |
| --- | --- |
| `T.M`, value receiver | `T::M`, a value of type `fn<R(T, A...)>` |
| `(*T).M`, pointer receiver | `T::M`, a value of type `fn<R(ptr<T>, A...)>` |
| `(*T).M`, value receiver | `[](ptr<T> _eo_receiver, A _eo_arg_0) { return T::M(eo::value_receiver(_eo_receiver, "M"), _eo_arg_0); }` |

For a method translated as a coroutine, the result in each type is `func<R>`, as in `fn<func<R>(T, A...)>`, and a method value of it declared with its type is `fn<func<R>(A...)>`.

`eo::value_receiver(p, "M")` returns `*p`. For a nil `p` it panics as Go does, with the `runtime.Error` `value method main.T.M called using nil *T pointer`, built from `T::_eo_name`.

#### Coroutine function values

A function value is a coroutine value when it is a function or a function literal translated as a coroutine (see [Blocking operations](#blocking-operations)), a method value of such a method, or a value read from a coroutine slot. A slot is a variable, a parameter, a result, or a struct field of function type, or a function type inside the type of one of them, such as the element of `[]func()`, `map[K]func()`, or `chan func()`, or a parameter or result of `func(func())`.

A slot is a coroutine slot, `fn<func<R>(A...)>`, when a coroutine value can be stored in it, by an initialization, an assignment, an argument, a result, a send, or an element of a composite literal. Every other slot is plain, `fn<R(A...)>`. The decision is made over the whole program and repeated until no slot changes, since storing a value read from a slot that has become a coroutine slot makes the target one too. A function type inside another type is one decision for every value that flows between two slots: assigning a `[]func()` to another `[]func()` variable makes both element types coroutine slots if either is, because `slice<fn<void()>>` and `slice<fn<func<>()>>` are different C++ types.

Calling a coroutine slot is `co_await f(args)`, so a function that calls one is translated as a coroutine. Calling a plain slot is a plain call.

```go
func run(f func(int) int, v int) int { return f(v) }

send := func(v int) int { ch <- v; return v + 1 }
run(send, 3)
run(twice, 4) // func twice(n int) int
```

```cpp
auto run(fn<func<int64_t>(int64_t)> f, int64_t v) -> func<int64_t> {
  co_return co_await f(v);
}

auto send = [=](int64_t v) mutable -> func<int64_t> {
  co_await (ch << v);
  co_return v + 1;
};
co_await run(send, 3);
co_await run(twice, 4);
```

A plain value stored in a coroutine slot, such as `twice` above, is adapted by `fn`. It runs when it is called, and the awaitable completes without suspending. A plain `fn` converts to a coroutine `fn` of the same signature too, and a nil one stays nil. A coroutine value does not convert to a plain slot. The conversion does not compile, so a slot decided wrongly is a compile error, never an awaitable that a plain call drops without running it. A call of a coroutine slot without `co_await` is diagnosed too, because its `operator()` is `[[nodiscard]]`.

A `defer` of a call through a coroutine slot is unsupported, as is any deferred coroutine call (see [Blocking operations](#blocking-operations)).

#### Function values not covered yet

- a function value converted to an interface, and type assertions and type switches on a function type;
- printing a function value with `fmt`, which Go prints as an address, or `<nil>`;
- variadic function types, such as `func(...int)`;
- method values and method expressions of interfaces ([#67](https://github.com/conr2d/eo/issues/67)), and of methods translated as coroutines that an interface declares ([#108](https://github.com/conr2d/eo/issues/108)).

### Interfaces

A Go interface type maps to a C++ class derived from `eo::interface<I>`, from `<eo/interface.h>`, which `<eo/fmt.h>` includes. An interface value is nil, or holds a dynamic value and its type, as in Go. Satisfaction stays implicit: each translated type lists its methods in a table, and a conversion to an interface looks up the interface's methods in that table by name and signature, as Go's runtime does with its method tables. A conversion that Go checks at compile time is checked at compile time: it does not compile if the type lacks one of the methods. A type assertion, a type switch, or a conversion from one interface to another looks the methods up at run time and keeps the result, as Go's itab cache does.

#### Method tables

Every package-level struct type declares its Go type name as its first member, and a [defined type](#defined-types) right after `using defined::defined;`. The name is the package name and the type name joined by a dot, as Go's runtime prints it:

```cpp
static constexpr std::string_view _eo_name = "main.Point";
```

A type whose method set, or whose pointer type's method set, is not empty declares the table of its methods as its last member: `using _eo_methods = eo::methods<...>;`, with one `eo::method<&T::M, "M">` for each static member function that translates a method, in the order the struct declares them. `"M"` is the Go method name. `eo::methods` and `eo::method` are always qualified, because a Go program can name a field `method`. A method whose receiver is `T` belongs to the method sets of `T` and of `ptr<T>`, and a method whose receiver is `ptr<T>` only to that of `ptr<T>`, as in Go. Calling a value-receiver method through an interface that holds a `ptr<T>` dereferences the pointer, which panics for nil, as Go's wrapper method does.

A method promoted through an embedded field is in the table through a forwarder: a static member function with the method's name, parameters, and result, whose receiver parameter `_eo_receiver` is `T` if `go/types` puts the method in the method set of `T`, and `ptr<T>` if only in that of `*T`. The forwarders follow the struct's own methods, ordered by name. A forwarder returns the call of the promoted method on the path to its embedded field, with `_eo_receiver` as the receiver, as in [Embedded fields](#embedded-fields). A type named by an embedded field is also the name of a member, so it is written with `::`:

```go
type Item struct {
	Base     // func (b Base) Describe() string, func (b *Base) Rename(name string)
	*Counter // func (c *Counter) Inc() int
	Labeler  // an interface with Label() string
}
```

```cpp
struct Item {
  static constexpr std::string_view _eo_name = "main.Item";

  ::Base Base{};
  ptr<::Counter> Counter{};
  ::Labeler Labeler{};

  auto operator==(const Item&) const -> bool = default;

  static auto Describe(Item _eo_receiver) -> std::string;
  static auto Inc(Item _eo_receiver) -> int64_t;
  static auto Label(Item _eo_receiver) -> std::string;
  static void Rename(ptr<Item> _eo_receiver, std::string name);

  using _eo_methods = eo::methods<eo::method<&Item::Describe, "Describe">, eo::method<&Item::Inc, "Inc">,
    eo::method<&Item::Label, "Label">, eo::method<&Item::Rename, "Rename">>;
};

auto Item::Describe(Item _eo_receiver) -> std::string {
  return ::Base::Describe(_eo_receiver.Base);
}

auto Item::Inc(Item _eo_receiver) -> int64_t {
  return ::Counter::Inc(_eo_receiver.Counter);
}

auto Item::Label(Item _eo_receiver) -> std::string {
  return _eo_receiver.Labeler.Label();
}

void Item::Rename(ptr<Item> _eo_receiver, std::string name) {
  return ::Base::Rename(ptr(_eo_receiver, &_eo_receiver->Base), name);
}
```

A call of a promoted method in the program is still written on the path to its embedded field, as in [Embedded fields](#embedded-fields). Only the table uses the forwarders.

A struct type declared inside a function cannot have static data members in C++. It has no `_eo_name` and no forwarders, and converting it to an interface is not covered yet.

#### Interface types

An interface type is a class named after it, derived from `interface<I>`, which inherits its constructors and declares `_eo_name` like a struct. Each method of the interface is a `const` member function with the method's parameters and result, declared in the class and defined after the type definitions (see [Declaration order](#declaration-order)), because its parameter and result types may not be complete inside the class. Its body returns `_eo_call<&I::M>(args...)` with the parameters in order. An unnamed parameter is named `_eo_arg_<n>` (see [Methods](#methods)). The class declares the methods in the order the interface declares them, and `_eo_methods` lists them in that order. An embedded interface contributes its methods at its position, in its own order: the class declares and defines each of them as its own member function, as if the interface had declared it. A method that Go merges from two embedded interfaces is declared and listed once, at its first position. `type ReadWriter interface { Reader; Writer }` thus declares `Read`, then `Write`.

```go
type Shape interface {
	Area() float64
	Scale(k float64)
}
```

```cpp
struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Shape";

  auto Area() const -> double;
  void Scale(double k) const;

  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">, eo::method<&Shape::Scale, "Scale">>;
};

auto Shape::Area() const -> double {
  return _eo_call<&Shape::Area>();
}

void Shape::Scale(double k) const {
  return _eo_call<&Shape::Scale>(k);
}
```

Go's `any` and `interface{}` map to `any`, Eo's empty interface, whose name is `interface {}`. Go's predeclared `error` maps to `error` (see [Errors](#errors)), and `fmt.Stringer` to `fmt::Stringer`. A package-level name that hides `any`, or a member named `any`, makes it `eo::any`, as for other Eo names (see [Package-level names hidden by members](#package-level-names-hidden-by-members)).

#### Interface values

| Go | C++ |
| --- | --- |
| `var x I` | `I x{};` |
| `nil` as an interface value | `nullptr` where it converts, and `I{}` otherwise |
| a conversion of `v` to `I`, implicit or written `I(v)` | `v` where C++ converts implicitly: an initializer, an assignment, an argument, a result, or an element of a braced list. `I(v)` where Go writes `I(v)`, and for an operand of `==` or `!=` |
| `x == nil`, `x != nil` | `x == nullptr`, `x != nullptr` |
| `x == y`, where both are interface values | `x == y` |
| `x == v`, `v == x`, where `v` is not an interface value | `x == I(v)`, `I(v) == x`, `I` being the type of `x` |
| `x.M(args)` | `x.M(args)` |

A conversion from a non-interface value holds a copy of the value, as in Go. Copies of an interface value share that copy, which is never modified, so they behave like Go's copies. An interface that holds a pointer shares the pointer's target. A variable whose address is converted to an interface is usually moved to heap by Go, and so [boxed](#boxed-variables), so the interface holds an owning pointer: `var p Point; var s fmt.Stringer = &p` is `auto p = New<Point>();`, then `fmt::Stringer s = p;`. An untyped constant converted to an interface takes its default type and is written `T{L}`, as described in [Untyped constants](#untyped-constants): `var a any = 7` is `any a = int64_t{7};`.

The dynamic type is the C++ type of the converted value, which must be a [basic type](#basic-types), a string, `eo::empty_struct`, a type that declares `_eo_name`, or a pointer, slice, or map of such types. Go's `int` and `int64` both map to `int64_t`, and `uint`, `uint64`, and `uintptr` to `uint64_t`, so an interface cannot tell them apart: an `int64` value converted to an interface holds an `int`. A program whose behavior depends on telling them apart, by an assertion, a type switch, or a comparison of interface values, is outside the mapping ([#107](https://github.com/conr2d/eo/issues/107)).

Calling a method of a nil interface value panics with `runtime error: invalid memory address or nil pointer dereference`, which a deferred `recover()` can stop. An interface value that holds a nil pointer is not nil, as in Go. Its pointer-receiver methods receive the nil pointer, and its value-receiver methods panic as above.

Two interface values are equal when both are nil, or when their dynamic types are identical and their values are equal. Values of different dynamic types are unequal. Comparing two values of the same uncomparable dynamic type, such as a slice, a map, or a struct whose `operator==` is deleted, panics with Go's `runtime error: comparing uncomparable type main.T`. A struct with an interface field compares it this way, and so can panic, as in Go.

#### Type assertions

| Go | C++ |
| --- | --- |
| `x.(T)` | `eo::type_assert<T>(x)` |
| `v, ok := x.(T)`, where both variables are new | `auto [v, ok] = eo::type_assert_ok<T>(x);` |
| `v, ok = x.(T)` | `std::tie(v, ok) = eo::type_assert_ok<T>(x);` |
| `v, _ := x.(T)`, `_, ok := x.(T)` | `auto v = eo::type_assert_ok<T>(x).first;`, `auto ok = eo::type_assert_ok<T>(x).second;` |
| `v, _ = x.(T)`, `_, ok = x.(T)` | `v = eo::type_assert_ok<T>(x).first;`, `ok = eo::type_assert_ok<T>(x).second;` |

`T` is the translated type, a non-interface type or an interface type. `eo::type_assert_ok` returns a `std::pair` of the value and a `bool`, and on failure the zero value of `T` and `false`. `std::tie` needs `<tuple>`. A `:=` that declares only some of its variables, or a variable that is [boxed](#boxed-variables), declares each new variable first, as `T v{};` or `auto v = New<T>();`, and then uses the `std::tie` form, with `*v` for a boxed `v`.

A failed `x.(T)` panics with Go's message, which a deferred `recover()` can stop:

- `interface conversion: interface {} is string, not int`, naming the static type of `x`, its dynamic type or `nil`, and `T`, for a non-interface `T`;
- `interface conversion: main.Rect is not main.Shape: missing method Scale`, naming the first missing method in name order, for an interface `T`;
- `interface conversion: interface is nil, not main.Shape`, for a nil `x` and an interface `T`.

#### Type switches

A type switch maps to an `if` statement whose init statement saves the guard in `_eo_type_switch`, followed by an `else if` for each clause in source order and an `else` for `default`, wherever Go writes it. A clause's condition is `_eo_type_switch == nullptr` for `nil`, `eo::holds<T>(_eo_type_switch)` for a type `T`, and those conditions joined by `||` for a clause with several types. Go checks the clauses in source order and takes the first that matches, and so does the `if` chain. A clause that uses the variable of `switch v := x.(type)` declares it first: `auto v = eo::type_assert<T>(_eo_type_switch);` in a clause with a single type `T`, and `auto v = _eo_type_switch;` in the other clauses, `case nil:` and `default:` included, where Go gives `v` the guard's type:

```go
switch v := x.(type) {
case nil:
	fmt.Println("nil")
case int, string:
	fmt.Println("int or string", v)
case Rect:
	fmt.Println("rect", v.W)
default:
	fmt.Println("other")
}
```

```cpp
if (auto _eo_type_switch = x; _eo_type_switch == nullptr) {
  fmt::Println("nil"s);
} else if (eo::holds<int64_t>(_eo_type_switch) || eo::holds<std::string>(_eo_type_switch)) {
  auto v = _eo_type_switch;
  fmt::Println("int or string"s, v);
} else if (eo::holds<Rect>(_eo_type_switch)) {
  auto v = eo::type_assert<Rect>(_eo_type_switch);
  fmt::Println("rect"s, v.W);
} else {
  fmt::Println("other"s);
}
```

The variable of each clause is a new variable, which is boxed by the same rules as any other. Go reports `moved to heap: v` for it at the position of the clause's first type, such as `int` in `case int:`, of `nil` in `case nil:`, or of the `default` keyword, and that report applies to the variable of that clause. A `default` clause that is not last in Go is still the final `else`, which does not change which clause runs. A type switch with an init statement, as in `switch y := f(); v := y.(type)`, puts the init statement in a block of its own, `{ auto y = f(); if (...) ... }`. A `break` that leaves a type switch becomes a `goto`, as in an expression switch (see [Switch statements](#switch-statements)).

#### Printing methods

`fmt::Println` and the other printing functions (see [Formatted printing](#formatted-printing)) print an interface value as its dynamic value, and a nil interface value as `<nil>`. They print a value whose method set has `Error() string` by calling `Error`, and otherwise one whose method set has `String() string` by calling `String`, as Go's fmt does, for the verbs `%v`, `%s`, `%x`, `%X`, and `%q`. This applies to operands, to elements of slices and maps, and to struct fields, and the method set is the static type's: a `T` whose `String` has a pointer receiver prints as a struct, and a `ptr<T>` calls `String`. A panic in the method prints `%!v(PANIC=String method: message)`, with the verb in place of `v`, or `<nil>` if the receiver is a nil pointer, as in Go. The message is the panic value as `%v` prints it, which calls its own `Error` or `String` method.

A value whose method set has `Format(f fmt.State, verb rune)` is a `fmt.Formatter`, and Go calls its `Format` in place of the other methods, for every verb, before `GoString` for `%#v`. The method is `static void Format(T t, fmt::State f, int32_t verb)`, as any method is written (see [Methods](#methods)). `fmt::State` and `fmt::Formatter` are interfaces from `<eo/fmt.h>`:

- `f.Write(b)` appends `b` to the text being printed, without padding. A `fmt::State` is an `io::Writer`, so `fmt.Fprintf(f, "%x", v)` is `fmt::Fprintf(f, "%x"s, v)`.
- `f.Width()` and `f.Precision()` return `std::tuple<int64_t, bool>`, the number and whether the directive has one. `f.Flag(c)` reports the flag `c`, which is `true` for `+` and `#` of `%+v` and `%#v`.
- `fmt.FormatString(f, verb)` is `fmt::FormatString(f, verb)`, the directive rebuilt from the state, as `%-8.3x`.
- The dynamic type of the state is `*fmt.pp`, as `%T` prints it. It refers to the call that prints, so it is valid only while `Format` runs, as in Go.
- A panic in `Format` keeps what it wrote and adds `%!v(PANIC=Format method: message)`, and a nil pointer receiver adds `<nil>`, as for the other methods.

Go calls these methods only for exported struct fields, as a value that `reflect` reads from an unexported field cannot be passed through an interface. The same holds for the values inside such a field: its elements, its own fields, and the dynamic value of an interface it holds. Eo follows this, and prints them by their kinds. A field is exported when its name starts with an ASCII upper-case letter. A field whose name starts with another upper-case letter, which Go also exports, is not covered yet.

#### Interfaces not covered yet

- methods translated as coroutines, whose signatures differ from the interface's ([#108](https://github.com/conr2d/eo/issues/108));
- method values and method expressions of interfaces, and `go` and `defer` launches of interface method calls ([#67](https://github.com/conr2d/eo/issues/67));
- interface values as map keys (see [Maps not covered yet](#maps-not-covered-yet));
- generic interfaces and type constraints, such as `comparable` and `~int | ~string`;
- interface types declared inside a function, local struct types held in an interface, and methods with unexported names of interfaces from other packages;
- comparing interface values that hold a struct with a `sync.Mutex` or another Eo library type without `==`, which Go compares and Eo reports as uncomparable;
- a pointer to an interface, `*I`, beyond holding it, comparing it, and printing it as an address.

### Multiple results

A function or method with more than one result returns a `std::tuple` of the translated result types, in order, from `<tuple>`. A coroutine returns `func<std::tuple<...>>`. A function with one result keeps that result's type. A method of an interface type returns the same type as the methods that implement it (see [Interface types](#interface-types)).

| Go | C++ |
| --- | --- |
| `func f() (int, string)` | `auto f() -> std::tuple<int64_t, std::string>` |
| `return a, b` | `return {a, b};`, and `co_return std::tuple<T0, T1>{a, b};` in a coroutine |
| `return g()`, where `g` has several results | `return g();` |
| `v, w := f()`, `var v, w = f()` | `auto [v, w] = f();` |
| `v, w = f()` | `std::tie(v, w) = f();` |
| `v, _ := f()`, `_, w := f()` | `auto v = std::get<0>(f());`, `auto w = std::get<1>(f());` |
| `v, _ = f()`, `_, w = f()` | `v = std::get<0>(f());`, `w = std::get<1>(f());` |
| `_, _ = f()` | `f();` |
| `h(f())` | `std::apply(h, f())` |

`f()` stands for any call with several results, including a method call, an interface method call, and `co_await g()` for a coroutine `g`. A `return` is `co_return` in a coroutine, as for any result.

`return {a, b};` initializes the tuple from a braced list, which C++ evaluates left to right, as Go evaluates the operands of `return`. A coroutine names the tuple type, because `co_return` passes its operand to a function template of `func<T>`, which cannot deduce a type from a braced list. For [Evaluation order](#evaluation-order), the list of a `return` is one full expression with no outermost operation.

A function with named results declares each of them at the start of its body, as `T name{};` in order, whether or not it has a `defer` statement. A named result that is [boxed](#boxed-variables) is declared as `auto name = New<T>();` and returned as `*name`, and a result named `_` is an unnamed result, `_eo_result_N`. A bare `return` returns them, as `return {q, r};` for several results and `return q;` for one. A function with a `defer` statement also assigns them differently, as described in [Defer](#defer).

#### Declaring and assigning several results

A `:=`, a `var` without a type, or an assignment `=` whose right side is one call with several results takes the first of these forms that applies:

1. Exactly one operand on the left is not `_`: the `std::get<I>` form, where `I` is that operand's position, from 0. A boxed variable is declared as `auto v = New<T>(std::get<I>(f()));`, and assigned as `*v = std::get<I>(f());`.
2. A `:=` or `var` whose operands on the left are all new variables, none of them `_` or [boxed](#boxed-variables): a structured binding, `auto [v, w] = f();`.
3. Otherwise, each new variable is declared first, in order, as `T v{};`, or `auto v = New<T>();` when it is boxed, and the `std::tie` form then assigns every operand. `_` is `std::ignore`, and a boxed `v` is `*v`. This covers a `:=` that declares only some of its variables, a `:=` with `_` and at least two other operands, and `var v, w T = f()`, which names the type.

```go
a, err := f()
b, err := g() // err is not new
k, _, n := h()
```

```cpp
auto [a, err] = f();
int64_t b{};
std::tie(b, err) = g();
int64_t k{};
std::string n{};
std::tie(k, std::ignore, n) = h();
```

A structured binding's names are ordinary variables for the other rules: a lambda copies one with `[=]` or captures it by reference with `[&v]`, and `ref(v)` takes its address. A `:=` in the init statement of an `if` or `switch` declares new variables only, so it uses form 1 or 2: `if _, ok := f(); ok` is `if (auto ok = std::get<1>(f()); ok)`, and `if v, ok := f(); ok` is `if (auto [v, ok] = f(); ok)`. An assignment there keeps its form: `if v, err = f(); err != nil` is `if (std::tie(v, err) = f(); err != nullptr)`.

The comma-ok form of a type assertion follows the same forms with a `std::pair`, whose single-operand form is written with `.first` and `.second` (see [Type assertions](#type-assertions)).

#### Calls with the results of another call

Go's `h(f())` passes each result of `f` as an argument of `h`. It is `std::apply(h, f())`, where `h` is a package-level function, or an Eo library function such as `fmt.Println`, which is a function object for this reason: `fmt.Println(f())` is `std::apply(fmt::Println, f())`. A coroutine `h` is called as `co_await std::apply(h, f())`, and a coroutine `f` as `std::apply(h, co_await f())`. For [Evaluation order](#evaluation-order), `std::apply(h, f())` is the call of `h`.

`go h(f())` and `defer h(f())` evaluate `f()` when the statement runs. Each argument temporary is declared with the value type of the parameter of `h` it is passed to, as `go` temporaries are (see [Argument temporary types](./GOROUTINE.md#argument-temporary-types)), and is then assigned from `f()` by the `std::tie` form. When `h` is an Eo library function object, such as `fmt::Println`, whose parameters are templates, each temporary takes the type of the result of `f` it holds:

```go
defer show(pair(n)) // func show(n int, s string)
```

```cpp
int64_t _eo_defer_arg_0_0{};
std::string _eo_defer_arg_0_1{};
std::tie(_eo_defer_arg_0_0, _eo_defer_arg_0_1) = pair(n);
eo_defer([=] { show(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
```

`go show(pair(n))` declares `_eo_go_arg_0_0` and `_eo_go_arg_0_1` the same way and launches `go([=]() mutable { return show(_eo_go_arg_0_0, _eo_go_arg_0_1); });`.

#### Multiple results not covered yet

- an assignment or declaration of several values that are not one call, such as `a, b = b, a` and `x, y := 1, 2` ([#111](https://github.com/conr2d/eo/issues/111)). A package-level `var` follows [Package initialization](#package-initialization) instead;
- an assignment `=` whose left side has an operand other than a variable or `_`, such as `x.f`, `*p`, or `s[i]`: Go evaluates the index and pointer operands on the left before the call and assigns the operands left to right, while C++ evaluates the operands of `std::tie` after the call, so a nil `*p` or an index out of range would panic before the operands to its left are assigned;
- an assignment that names the same variable twice, as in `x, x = f()`, where Go keeps the last value and `std::tuple` does not specify the order in which it assigns its elements;
- a `:=` in the init statement of an `if` or `switch` that would need form 3, because one of its variables is boxed or it has `_` and at least two other operands, and a `for` init statement with several results;
- `return a, b` in a function without a `defer` statement whose named results a function literal or a pointer can reach: Go assigns the named results before returning, and a goroutine that shares them can observe it, while `return {a, b};` leaves them unchanged;
- `h(f())` where `h` is a method, a function value, or a variadic Go function, and `go` or `defer` statements of such calls.

### Errors

Go's predeclared `error` is the interface `error` from `<eo/interface.h>`, whose method is `auto Error() const -> std::string`. It follows [Interfaces](#interfaces): `var err error` is `error err{};`, `err != nil` is `err != nullptr`, a `nil` result is `nullptr`, and a value whose method set has `Error() string` converts to it. A function with the results `(T, error)` returns `std::tuple<T, error>` (see [Multiple results](#multiple-results)). A nil pointer converted to `error` is not nil, as in Go. A translated error type is a struct whose method set has `Error`, declared as in [Methods](#methods) and [Method tables](#method-tables).

Go's `errors` package is `errors::` from `<eo/errors.h>`, and `fmt.Errorf` is `fmt::Errorf` from `<eo/fmt.h>`, which includes `<eo/errors.h>`:

| Go | C++ |
| --- | --- |
| `errors.New(s)` | `errors::New(s)` |
| `errors.Is(err, target)` | `errors::Is(err, target)` |
| `errors.As(err, &target)` | `errors::As(err, p)`, `p` being the translation of `&target` |
| `errors.Unwrap(err)` | `errors::Unwrap(err)` |
| `errors.Join(a, b)`, `errors.Join(errs...)` | `errors::Join(slice<error>::of({a, b}))`, `errors::Join(errs)` |
| `fmt.Errorf(format, a, b)` | `fmt::Errorf(format, a, b)` |

`errors.Join` is variadic, so its parameter is a `slice<error>`, as for a translated variadic function (see [Slices](#slices)). Go moves the target of `errors.As` to heap, because `As` takes it as `any`, so the target is usually [boxed](#boxed-variables) and `&target` is the box: `var nf *NotFound` followed by `errors.As(err, &nf)` is `auto nf = New<ptr<NotFound>>();` and `errors::As(err, nf)`, and `nf.Key` is then `(*nf)->Key`. An unboxed target is `ref(target)`.

These behave as in Go:

- `errors.New` returns a new `*errors.errorString` for each call, so two errors with the same text are not equal;
- `errors.Is` and `errors.As` visit the error and the errors that its `Unwrap() error` or `Unwrap() []error` method returns, depth first, and call its `Is(error) bool` or `As(any) bool` method. `errors.Is` compares an error with the target by `==` only when the target's dynamic type is comparable. `errors.As` stores the first error that a `T` can hold, a `T` being the target's type, an interface type or a type whose method set has `Error`;
- `errors.Unwrap` calls only an `Unwrap() error` method;
- `errors.Join` returns nil when every error is nil. Its `Error` joins the messages of the others with newlines;
- `fmt.Errorf` formats its message as [Formatted printing](#formatted-printing) describes. Without `%w` it returns an `*errors.errorString`. With one `%w` it returns an `*fmt.wrapError`, whose `Unwrap` returns the operand, and with several an `*fmt.wrapErrors`, whose `Unwrap` returns the operands that are errors. A `%w` whose operand is not an error prints as `%!w(int=3)`, and the error wraps nil in its place.

`errors.As` with a nil target panics with `errors: target must be a non-nil pointer`. A target whose type is neither an interface nor has `Error` in its method set does not compile, where Go panics.

#### Errors not covered yet

- `errors.ErrUnsupported`, and `Unwrap`, `Is`, and `As` methods translated as coroutines ([#108](https://github.com/conr2d/eo/issues/108)).

### Formatted printing

Go's `fmt.Print`, `fmt.Println`, `fmt.Printf`, `fmt.Fprint`, `fmt.Fprintln`, `fmt.Fprintf`, `fmt.Sprint`, `fmt.Sprintln`, `fmt.Sprintf`, `fmt.Append`, `fmt.Appendln`, `fmt.Appendf`, and `fmt.Errorf` are function objects of the same names in `fmt::`, from `<eo/fmt.h>`, so `fmt.Println(f())` is `std::apply(fmt::Println, f())` (see [Calls with the results of another call](#calls-with-the-results-of-another-call)). Their operands are C++ arguments, not a `slice<any>`: `fmt.Printf("%d %s\n", n, s)` is `fmt::Printf("%d %s\n"s, n, s)`. A `nil` operand is `nullptr`, and a numeric constant operand takes its default type, as in [Untyped constant arguments](#untyped-constant-arguments).

- `fmt::Print`, `fmt::Println`, and `fmt::Printf` write to `stdout`. `fmt::Fprint`, `fmt::Fprintln`, and `fmt::Fprintf` write to an `io::Writer`, such as a `fmt::State`, a `ptr<strings::Builder>`, or a `ptr<bytes::Buffer>`, with one `Write` of the whole text, or to a `std::FILE*`: `fmt.Fprintln(os.Stderr, x)` is `fmt::Fprintln(stderr, x)`, and `os.Stdout` is `stdout` there. Each call to a `std::FILE*` writes its whole text and flushes it, as Go writes to the unbuffered `os.Stdout`.
- The six `Print` and `Fprint` functions return `std::tuple<int64_t, error>`, Go's `(n int, err error)`: the result of `Write`, or the length of the text and `nullptr` for a `std::FILE*`. A `nil` writer, `nullptr`, formats the text and then panics with Go's nil dereference error, as Go's call of `Write` does. `n, err := fmt.Fprintf(w, "%d", x)` is `auto [n, err] = fmt::Fprintf(w, "%d"s, x);`, and a call whose results are not used is a statement.
- `fmt::Append`, `fmt::Appendln`, and `fmt::Appendf` take a `slice<uint8_t>` and return it with the text appended, as `eo::append_slice` does. `fmt.Append(nil)` stays nil.

All of them print an operand as Go's fmt does, with these verbs:

- `%v`, which is how `Println` prints;
- `%d`, `%b`, `%o`, `%O`, `%x`, `%X`, `%c`, `%q`, and `%U` for an integer, `%q` giving a quoted rune;
- `%b`, `%e`, `%E`, `%f`, `%F`, `%g`, `%G`, `%x`, and `%X` for a float;
- `%s`, `%q`, `%x`, and `%X` for a string, a `[]byte`, and a `[N]byte`;
- `%t` for a `bool`;
- `%p` for a pointer, a slice, and a map;
- `%T`, the Go type name, `%%`, and `%w` in `fmt::Errorf`.

A verb that the list above does not give for the operand's kind prints the bad-verb form described below. An array, a slice, a struct, and a pointer operand to an array, a struct, or a slice apply the verb to each element or field, as Go does: `fmt.Sprintf("%x", []int{10, 11})` is `[a b]`. A value whose method set has `Format(fmt.State, rune)`, `Error() string`, or `String() string` is printed by calling it, as in [Printing methods](#printing-methods). Errors in the format print as in Go: `%!d(string=s)` for a verb that the operand does not take, with the operand printed by `%v` without its methods, `%!d(MISSING)` for a missing operand, `%!(EXTRA string=x, float64=2.5)` for unused operands, `%!(NOVERB)` for a `%` at the end, and `%!w(...)` for `%w` outside `fmt::Errorf`. `fmt.Sprint` puts a space between two operands when neither is a string, as Go does.

The flags, widths, precisions, and operand indexes are ported from Go's `fmt/format.go` and `fmt/print.go`, so they behave as in Go:

- The flags are `#`, `0`, `+`, `-`, and space. A width or precision is a number or `*`, which takes the next operand: an integer operand whose value is at most 1e6 in magnitude. A negative `*` width sets the `-` flag. `%!(BADWIDTH)` and `%!(BADPREC)` mark an operand that cannot be one.
- An operand index, as in `%[2]d` or `%[3]*.[2]*[1]f`, picks the operand for the verb, the width, or the precision, and `%!d(BADINDEX)` marks a bad one. A format with an index does not report unused operands.
- A width pads with spaces, or with zeros for the `0` flag, counting runes. A precision cuts a string to that many runes, and `%x` of a string or `[]byte` to that many bytes. For a float it sets the digits after the point for `%e` and `%f`, and the significant digits for `%g`. For an integer it sets the least number of digits.
- `+` prints the sign of a number and escapes `%q` to ASCII. `#` gives `0x`, `0X`, `0b`, or a leading `0` to an integer, a backquoted string to `%q` when it can be one, a decimal point to a float, with trailing zeros for `%g`, `%G`, and `%x` but not `%X`, as in Go, and `U+0078 'x'` to `%U`. Space puts a space before a number without a sign, and between the bytes of `%x`.
- The flags apply to each element or field of a value, and to the text of an `Error` or `String` method, as in Go: `fmt.Sprintf("%6v", point{1, 2})` is `{     1      2}`.
- `%+v` prints struct field names, as in `{X:1 Y:2}`.
- `%#v` prints Go syntax: `main.point{X:1, Y:2}`, `[]int{1, 2}`, `map[string]int{"a":1}`, `[]byte(nil)`, `0x2a` for an unsigned integer, a quoted string, and `error(nil)` for a nil interface inside another value. It calls a `GoString() string` method in place of `Error` and `String`. A defined slice, array, or map type prints with its own name, as in `main.Hashes{0xab}`.
- Floats are formatted as Go's `strconv.FormatFloat` formats them, from the digits of `std::to_chars`, so `%.2f` of `1.005` is `1.00` and `%.0f` of `2.5` is `2`.

Eo reads the field names for `%+v` and `%#v` from the compiler. Boost.PFR gives them only from Boost 1.84, so `<eo/print.h>` takes them, as PFR does, from the function signature that GCC and Clang print for a pointer to the field. A translated struct keeps its Go field names, so they print as in Go.

`%q` escapes the bytes that are not valid UTF-8, the ASCII control characters, and the code points that Go's `unicode.IsPrint` rejects among C1 controls, spaces other than U+0020, U+00AD, U+2000 through U+200F, U+2028 through U+202F, U+205F through U+2064, U+3000, U+FEFF, U+FFF9 through U+FFFB, noncharacters U+FFFE and U+FFFF, and private use. It prints other code points as they are.

#### Formatted printing not covered yet

- `%p` of a channel or a function, and complex numbers;
- `%#v` of an anonymous struct, a pointer inside another value, or a function, and the type names of `%#v` that Eo does not know, as for `%T`;
- the `Sscan` family, and a `fmt.State` that is not the one `Format` gets, such as one that a program implements;
- `os.Stdout` and `os.Stderr` other than as the writer of `fmt.Fprint`, `fmt.Fprintln`, and `fmt.Fprintf`, such as an `io.Writer` variable that holds one;
- `%v` of a `fmt.State`, which prints Go's whole `pp` struct;
- an error in writing to a `std::FILE*`, such as a closed pipe, which Go returns as `err` and Eo does not report;
- a `...any` argument passed on with `args...`, as in `fmt.Sprintf(format, args...)`;
- `%q` and `%#U` of an unassigned code point or a format character not listed above, which Go escapes, or prints without the character for `%#U`, and Eo prints as it is;
- `%T` of an `int64` value, which prints `int` ([#107](https://github.com/conr2d/eo/issues/107)).

### The math/big package

Go's `math/big` maps, in part, to the namespace `big` from `<eo/math/big.h>`. It covers the type `big.Int` with the methods below, `big.NewInt`, and `big.Word`. An `Int` holds its value in a Boost.Multiprecision `cpp_int`.

| Go | C++ |
| --- | --- |
| `big.Int`, `*big.Int` | `big::Int`, `ptr<big::Int>` |
| `new(big.Int)`, `var x big.Int`, `big.Int{}` | `New<big::Int>()`, `big::Int x{};`, `big::Int{}` |
| `big.NewInt(v)` | `big::NewInt(v)` |
| `z.Add(x, y)`, `x.Cmp(y)`, where `z` and `x` are pointers | `big::Int::Add(z, x, y)`, `big::Int::Cmp(x, y)` |
| `x, ok := z.SetString(s, base)` | `auto [x, ok] = big::Int::SetString(z, s, base);` |
| `big.Word`, `[]big.Word` | `big::Word`, `slice<big::Word>` |

The methods are static members whose first parameter is the receiver, as for a translated type in [Methods](#methods), and unlike the member functions of `sync::Mutex`. Go's methods return their receiver, as in `new(big.Int).Add(x, y)`, and only a `ptr<big::Int>` keeps the new `Int` alive. Every method has a pointer receiver, so a call follows the pointer-receiver column of the call table in [Methods](#methods): `z.Add(&z, &z)` of a boxed `z` is `big::Int::Add(z, z, z)`, and of an unboxed one is `big::Int::Add(ref(z), ref(z), ref(z))`. A chain is a nested call, and [Evaluation order](#evaluation-order) applies to it as to any other call, so `c.Sub(c, a).Mul(c, big.NewInt(3))` saves both of its nested calls:

```cpp
auto _eo_call_0 = big::Int::Sub(c, c, a);
auto _eo_call_1 = big::NewInt(3);
big::Int::Mul(_eo_call_0, c, _eo_call_1);
```

The covered methods are `Set`, `SetInt64`, `SetUint64`, `SetBytes`, `SetBits`, `SetString`, `SetBit`, `Neg`, `Abs`, `Add`, `Sub`, `Mul`, `Quo`, `Rem`, `QuoRem`, `Div`, `Mod`, `DivMod`, `Exp`, `ModInverse`, `Lsh`, `Rsh`, `And`, `AndNot`, `Or`, `Xor`, `Not`, `Cmp`, `CmpAbs`, `Sign`, `BitLen`, `TrailingZeroBits`, `Bit`, `Int64`, `Uint64`, `IsInt64`, `IsUint64`, `Bytes`, `FillBytes`, `Bits`, `Text`, and `String`. Their parameters and results are translated as any others are: Go's `int` is `int64_t`, its `uint` is `uint64_t`, and two results are a `std::tuple`, as in [Multiple results](#multiple-results). A `nil` modulus of `Exp` is `nullptr`. `big.Word` is a [defined type](#defined-types) whose underlying type is `uint64_t`, so its constants and conversions are written as those of any defined type, such as `big::Word{255}` and `big::Word(nib)`.

The methods give Go's results:

- `Quo` and `Rem` truncate toward zero. `Div` and `Mod` are Euclidean, so the modulus is never negative.
- `And`, `Or`, `Xor`, `AndNot`, `Not`, `Bit`, and `SetBit` act on the two's complement of a negative value, and `Rsh` of a negative value rounds toward negative infinity.
- `Exp` with a `nil` or zero modulus gives `x**y`, which is 1 when `y` is 0 or negative. A negative `y` with another modulus uses the inverse of `x`. `Exp` and `ModInverse` return `nil` and leave `z` unchanged when there is no inverse.
- `SetString` takes the base prefixes `0b`, `0o`, `0x`, and `0` when the base is 0, and `_` separators only then. It reads letters as digits in either case up to base 36, and as two separate ranges of digits above it. It returns `nil, false` for a string that is not a whole number.
- `Bytes` of 0 is an empty slice that is not nil, and `Bits` of 0 is nil.

The methods panic as Go's do, with a Go string value:

- `division by zero` from `Quo`, `Rem`, `QuoRem`, `Div`, `Mod`, `DivMod`, and `ModInverse`;
- `math/big: buffer too small to fit value` from `FillBytes`;
- `negative bit index` and `set bit is not 0 or 1` from `Bit` and `SetBit`;
- `invalid base` from `Text`, for a base other than 2 through 62;
- `invalid number base N` from `SetString`, for a base other than 0 and 2 through 62.

A nil receiver or operand panics with Go's nil dereference error. `String` and `Text` of a nil `*big.Int` give `<nil>`, as in Go.

Three operations copy where Go shares storage:

- `Bits` returns a copy of the words, so a write to them does not change the `Int`, as it does in Go.
- `SetBits` copies the words, where Go makes them the storage of `z`.
- Copying a `big.Int` value copies the number. Go copies the header of the slice that holds the magnitude, and `go vet` does not report it. A program that changes one copy and reads the other is outside this mapping.

A `*big.Int` is a `fmt.Formatter`, and `big::Int::Format` is Go's `Int.Format`, ported from `math/big/intconv.go`. `fmt` calls it for operands and inside slices, maps, struct fields, and interfaces:

- `%d`, `%s`, and `%v` print in decimal, and `%b`, `%o`, `%O`, `%x`, and `%X` print in their bases, as in `-deadbeef`;
- `+` and space print the sign of a value that is not negative, `#` gives `0b`, `0`, `0x`, or `0X`, and `%O` always gives `0o`;
- a precision sets the least number of digits, and a precision of 0 prints nothing for 0. A width pads with spaces on the left, on the right for `-`, or with zeros for `0` without a precision;
- a nil pointer prints `<nil>`, without padding;
- another verb prints `%!q(big.Int=10)`.

`%T` prints `*big.Int`. A `big.Int` that is not a pointer has neither `Format` nor `String`, so it prints as its struct, `{false [10]}`, with the verb and flags applied to the sign and each word, and `{neg:false abs:[10]}` for `%+v`, as in Go. `%#v` prints `big.Int{neg:false, abs:big.nat{0xa}}`, and `big.nat(nil)` for 0.

A `*big.Int` is a Go type for interfaces, compared by its pointer, and `String` is in its method table, so `x.(fmt.Stringer)` succeeds. A `big.Int` value is not comparable, because Go's holds a slice.

#### The math/big package not covered yet

- `big.Float`, `big.Rat`, and the functions and methods not listed above, such as `GCD`, `Sqrt`, `ProbablyPrime`, `Float64`, `Append`, `Scan`, and the `encoding` and `json` marshaling methods;
- method values and method expressions of `big.Int` methods, and methods other than `String` in an interface;
- `%w` of a `*big.Int` outside `fmt::Errorf`, which prints an address where Go prints `&{false [...]}`;
- `%#v` of a `big.Int` value of 0 that an operation has set, which Go prints `big.nat{}` and Eo `big.nat(nil)`;
- `QuoRem` and `DivMod` whose two results are the same `Int`, or whose modulus `m` is also `y`, where Go's result depends on the order of its own writes.

### The strconv package

Go's `strconv` maps, in part, to the namespace `strconv` from `<eo/strconv.h>`. Its functions are called as Go calls them, and its results follow [Multiple results](#multiple-results): `n, err := strconv.Atoi(s)` is `auto [n, err] = strconv::Atoi(s);`, and `strconv.FormatInt(x, 16)` is `strconv::FormatInt(x, 16)`. It covers these names:

- `ParseInt`, `ParseUint`, `Atoi`, and `ParseBool`;
- `Itoa`, `FormatInt`, `FormatUint`, and `FormatBool`;
- `FormatFloat` and `AppendFloat`;
- `AppendInt`, `AppendUint`, `AppendBool`, and `AppendQuote`;
- `Quote`, `QuoteToASCII`, `QuoteRune`, `QuoteRuneToASCII`, `CanBackquote`, `Unquote`, `UnquoteChar`, and `IsPrint`;
- the error type `NumError` and the variables `ErrRange` and `ErrSyntax`;
- the constant `IntSize`, which is 64.

The functions are ported from Go 1.26:

- The parse functions take base prefixes and `_` separators only for base 0. A value out of range gives the largest value of its sign, with an error.
- A failed parse returns a `*strconv.NumError` as its `error`. Its `Error` is Go's, as in `strconv.ParseInt: parsing "999": value out of range`, and it unwraps to `strconv::ErrRange`, `strconv::ErrSyntax`, or an error for an invalid base or bit size. So `errors::Is(err, strconv::ErrRange)` and `errors::As(err, p)` with a `ptr<strconv::NumError>` target work as in Go.
- `Unquote` and `UnquoteChar` return `strconv::ErrSyntax` itself.
- `FormatInt`, `FormatUint`, `AppendInt`, and `AppendUint` panic with `strconv: illegal AppendInt/FormatInt base` for a base other than 2 through 36.
- `FormatFloat` takes the formats `'b'`, `'e'`, `'E'`, `'f'`, `'g'`, `'G'`, `'x'`, and `'X'`, and a precision of -1 for the fewest digits that read back as the same value. Another format gives `%` followed by the format, and a bit size other than 32 or 64 panics with `strconv: illegal AppendFloat/FormatFloat bitSize`, as in Go.
- `Quote` and `IsPrint` decide which code points are printable as `%q` does (see [Formatted printing](#formatted-printing)), so an unassigned code point counts as printable.

#### The strconv package not covered yet

- `ParseFloat`, the complex functions, `QuoteToGraphic`, `AppendQuoteRune`, the `Append` forms of the ASCII quoting functions, the graphic quoting functions, `QuotedPrefix`, and `IsGraphic`.

### The encoding/hex package

Go's `encoding/hex` maps, in part, to the namespace `hex` from `<eo/encoding/hex.h>`. It covers `Encode`, `Decode`, `EncodeToString`, `DecodeString`, `EncodedLen`, `DecodedLen`, the variable `ErrLength`, and the defined type `InvalidByteError`. `hex.InvalidByteError(c)` is `hex::InvalidByteError(c)`, and a constant one is `hex::InvalidByteError{97}`, as for any [defined type](#defined-types).

The functions are ported from Go 1.26:

- `Decode` and `DecodeString` stop at the first invalid byte, and return the bytes decoded before it with a `hex::InvalidByteError`. Its message is Go's, as in `encoding/hex: invalid byte: U+0067 'g'`.
- An odd length gives `hex::ErrLength`, unless the last byte is invalid too, in which case it gives the invalid byte.
- `Encode` and `Decode` write to `dst` through Go's index checks, so a `dst` that is too short panics with Go's runtime error after the bytes before it are written.

#### The encoding/hex package not covered yet

- `AppendEncode`, `AppendDecode`, `Dump`, `Dumper`, `NewEncoder`, and `NewDecoder`.

### The encoding/binary package

Go's `encoding/binary` maps, in part, to the namespace `binary` from `<eo/encoding/binary.h>`. It covers:

- the byte orders `BigEndian` and `LittleEndian`;
- the interfaces `ByteOrder` and `AppendByteOrder`;
- `PutUvarint`, `Uvarint`, `AppendUvarint`, `PutVarint`, `Varint`, and `AppendVarint`;
- the constants `MaxVarintLen16`, `MaxVarintLen32`, and `MaxVarintLen64`.

`binary.BigEndian` and `binary.LittleEndian` are constants of the types `binary::bigEndian` and `binary::littleEndian`, which keep Go's unexported names. Their methods have value receivers, and they are written as for a translated type in [Methods](#methods): `binary.BigEndian.PutUint64(b, v)` is `binary::bigEndian::PutUint64(binary::BigEndian, b, v)`. The methods are `Uint16`, `Uint32`, `Uint64`, `PutUint16`, `PutUint32`, `PutUint64`, `AppendUint16`, `AppendUint32`, `AppendUint64`, `String`, and `GoString`. A byte order passed as a `binary.ByteOrder` or a `binary.AppendByteOrder` is an [interface value](#interface-values), whose methods are called as members: `order.Uint64(b)`.

The functions are ported from Go 1.26:

- `Uint64` and the other methods check the last index first, as Go's do, so a slice that is too short panics with Go's runtime error, such as `runtime error: index out of range [7] with length 7`, before anything is read or written.
- `PutUvarint` and `PutVarint` panic the same way at the first byte that does not fit.
- `Uvarint` and `Varint` return 0 bytes for a buffer that ends inside the value, and `-n` for a value that overflows 64 bits at byte `n`.

#### The encoding/binary package not covered yet

- `Read`, `Write`, `Size`, `Append`, `Encode`, `Decode`, `ReadUvarint`, `ReadVarint`, and `NativeEndian`.

### The strings and bytes packages

Go's `strings` and `bytes` map, in part, to the namespaces `strings` from `<eo/strings.h>` and `bytes` from `<eo/bytes.h>`. Their functions are called as Go calls them, and their results follow [Multiple results](#multiple-results): `before, after, found := strings.Cut(s, sep)` is `auto [before, after, found] = strings::Cut(s, sep);`.

A function argument, such as the `f` of `strings.FieldsFunc`, `strings.TrimFunc`, or `strings.Map`, is a function used as a value or a function literal, passed as it is. `strings.NewReplacer` is variadic, so its parameter is a `slice<std::string>`: `strings.NewReplacer("a", "b").Replace(s)` is `strings::NewReplacer(slice<std::string>::of({"a"s, "b"s}))->Replace(s)`.

`strings` covers these names:

- search: `Compare`, `Contains`, `ContainsAny`, `ContainsRune`, `Count`, `HasPrefix`, `HasSuffix`, `Index`, `IndexAny`, `IndexByte`, `IndexFunc`, `IndexRune`, `LastIndex`, `LastIndexByte`, and `LastIndexFunc`;
- splitting and joining: `Cut`, `CutPrefix`, `CutSuffix`, `Fields`, `FieldsFunc`, `Join`, `Split`, and `SplitN`;
- trimming: `Trim`, `TrimFunc`, `TrimLeft`, `TrimLeftFunc`, `TrimPrefix`, `TrimRight`, `TrimRightFunc`, `TrimSpace`, and `TrimSuffix`;
- transforming: `EqualFold`, `Map`, `Repeat`, `Replace`, `ReplaceAll`, `ToLower`, and `ToUpper`;
- `NewReplacer` with the method `Replace`;
- `Builder`.

`bytes` covers these names:

- search: `Compare`, `Contains`, `Count`, `Equal`, `EqualFold`, `HasPrefix`, `HasSuffix`, `Index`, `IndexAny`, `IndexByte`, and `LastIndex`;
- splitting and joining: `Cut`, `Join`, `Split`, and `SplitN`;
- copying and transforming: `Clone`, `Map`, `Repeat`, `Replace`, `ReplaceAll`, `ToLower`, and `ToUpper`;
- trimming: `Trim`, `TrimFunc`, `TrimLeft`, `TrimLeftFunc`, `TrimPrefix`, `TrimRight`, `TrimRightFunc`, `TrimSpace`, and `TrimSuffix`.

The functions are ported from Go 1.26. The code points of a string or a `[]byte` are decoded as `range` decodes them, so a byte that does not start a valid UTF-8 sequence is U+FFFD. This applies wherever a function looks at code points:

- `Map`, `ToUpper`, and `ToLower` write such a byte as the encoding of U+FFFD, as Go does.
- An empty separator splits after each UTF-8 sequence.
- An empty `old` in `Replace` matches at the start and after each UTF-8 sequence.

`strings.Replacer` keeps Go's rules. Replacements happen in the order of the target string, without overlapping. At one position, the pair that comes first in the arguments wins. An empty old string matches at every byte, but never twice in a row.

The functions panic with Go's messages, such as `strings: negative Repeat count`, `strings: Repeat output length overflow`, and `strings.NewReplacer: odd argument count`.

A `bytes` function that Go defines as returning part of its argument returns a slice of the same backing array, so a write through the result changes the argument. These are `Cut`, `Split`, `SplitN`, `TrimPrefix`, `TrimSuffix`, and the `Trim` functions. The part of `Split` before each separator has its capacity cut at its end, as in Go.

The other functions return new slices. The `bytes` functions also return nil and empty slices where Go's do:

- `Trim`, `TrimLeft`, `TrimLeftFunc`, `TrimFunc`, and `TrimSpace` return nil when they cut everything, and so do `Trim` and `TrimLeft` for an empty argument. `TrimRight` returns an empty slice of the argument.
- `Clone` of nil is nil. `Cut` gives a nil `after` when it does not find the separator. `Join` of one empty element is nil, and `Replace` returns nil when it replaces nothing in an empty argument.
- `Repeat` and `Join` of no elements give an empty slice that is not nil. So do `ToUpper`, `ToLower`, and `Map` of an empty or nil argument.

The capacity of a new slice follows [Slices](#slices) and is outside this mapping.

`strings.Builder` is `strings::Builder`. Its methods are member functions, as for the other Eo library types (see [Methods](#methods)): `var b strings.Builder` is `strings::Builder b;`, `b.WriteString(s)` is `b.WriteString(s)`, and `n, err := b.WriteString(s)` is `auto [n, err] = b.WriteString(s);`.

- The methods are `Write`, `WriteByte`, `WriteRune`, `WriteString`, `String`, `Len`, `Cap`, `Grow`, and `Reset`.
- A `ptr<strings::Builder>` has them in its method set, as a `*strings.Builder` does, so `&b` is an `io.Writer` and a `fmt.Stringer`. Go moves a `b` passed as `&b` to an `io.Writer` to the heap, so `b` is [boxed](#boxed-variables): `var b strings.Builder` is `auto b = New<strings::Builder>();`, `fmt.Fprintf(&b, "%d", n)` is `fmt::Fprintf(b, "%d"s, n)`, and `b.String()` is `b->String()`.
- `WriteRune` writes an invalid code point as U+FFFD.
- A `Builder` that is written to and then copied panics on the next write through the copy, with `strings: illegal use of non-zero Builder copied by value`, as Go's does.
- `Grow` panics on a negative count with `strings.Builder.Grow: negative count`.
- The value of `Cap` depends on allocation sizes, which Go rounds and Eo does not, so a program that prints it is outside this mapping.

#### The strings and bytes packages not covered yet

- the `io` methods of `Replacer`, such as `Replacer.WriteString`, and method values and method expressions of `Builder` methods;
- the iterator functions, such as `SplitSeq`, `FieldsSeq`, and `Lines`;
- `Title`, `ToTitle`, `ToValidUTF8`, `SplitAfter`, `SplitAfterN`, the `Special` case functions, `ContainsFunc`, `IndexFunc` and `LastIndexFunc` of `bytes`, `bytes.Runes`, and the other functions not listed above.

### The unicode package

Go's `unicode` maps, in part, to the namespace `unicode` from `<eo/unicode.h>`. It covers:

- `ToUpper`, `ToLower`, `ToTitle`, `SimpleFold`, and `IsSpace`;
- the constants `MaxRune`, `ReplacementChar`, `MaxASCII`, and `MaxLatin1`.

They compute their results as Go's `letter.go` and `graphic.go` do, from tables that `tools/unicode` generates from Go's own: `go run tools/unicode/main.go > src/eo/unicode/tables.h`. The tables are those of Unicode 15.0.0, which Go 1.26 uses. A parity case compares every code point's results with Go's.

#### The unicode package not covered yet

- the category and script tables and the functions that use them, such as `IsLetter`, `IsDigit`, `IsUpper`, `IsPrint`, and `In`, and `SpecialCase`.

### The unicode/utf8 package

Go's `unicode/utf8` maps to the namespace `utf8` from `<eo/unicode/utf8.h>`. It covers every function and constant of the package: `RuneError`, `RuneSelf`, `MaxRune`, and `UTFMax`; `FullRune`, `DecodeRune`, `DecodeLastRune`, `RuneCount`, and `Valid` with their `InString` and `String` forms; `EncodeRune`, `AppendRune`, `RuneLen`, `RuneStart`, and `ValidRune`.

A `rune` is `int32_t` and an `int` result is `int64_t`, so `r, size := utf8.DecodeRune(b)` is `auto [r, size] = utf8::DecodeRune(b);`. The functions are ported from Go 1.26's `utf8.go`, with its tables, so an invalid or truncated encoding decodes to `RuneError` with the size Go gives, a surrogate or a value above `MaxRune` encodes as `RuneError`, and `RuneLen` of either is `-1`. `EncodeRune` panics with `runtime error: index out of range` when its slice is too short for the encoding, and writes nothing then, as Go's does. `Valid` checks one byte at a time where Go checks a word of ASCII at a time, which gives the same results.

### The math/bits package

Go's `math/bits` maps to the namespace `bits` from `<eo/math/bits.h>`. It covers:

- `LeadingZeros`, `TrailingZeros`, `OnesCount`, `Len`, `RotateLeft`, and `Reverse`, in each width;
- `ReverseBytes` in its widths;
- `Add`, `Sub`, `Mul`, `Div`, and `Rem` with their 32-bit and 64-bit forms;
- the constant `UintSize`, which is 64.

Go's `uint` is `uint64_t`, and its `int` results are `int64_t`. Two results are a `std::tuple`, so `sum, carry := bits.Add64(x, y, 0)` is `auto [sum, carry] = bits::Add64(x, y, 0);`. A negative `RotateLeft` count rotates right. `Div` and `Rem` panic with `runtime error: integer divide by zero` for a zero divisor, as Go's do. `Div` also panics with `runtime error: integer overflow` when the quotient does not fit, and `Rem` does not.

### The io package

Go's `io` maps, in part, to the namespace `io` from `<eo/io.h>`. It covers:

- the interfaces `Reader`, `Writer`, `Closer`, `Seeker`, `ReadWriter`, `ReadCloser`, `WriteCloser`, `ReadWriteCloser`, `ReadSeeker`, `ReadSeekCloser`, `WriteSeeker`, `ReadWriteSeeker`, `ReaderFrom`, `WriterTo`, `ReaderAt`, `WriterAt`, `ByteReader`, `ByteScanner`, `ByteWriter`, `RuneReader`, `RuneScanner`, and `StringWriter`;
- the errors `EOF`, `ErrUnexpectedEOF`, `ErrShortWrite`, `ErrShortBuffer`, `ErrNoProgress`, and `ErrClosedPipe`, and the constants `SeekStart`, `SeekCurrent`, and `SeekEnd`;
- `ReadFull`, `ReadAtLeast`, `ReadAll`, `Copy`, `CopyN`, `CopyBuffer`, `WriteString`, `Discard`, `NopCloser`, `LimitReader`, `MultiReader`, `MultiWriter`, `TeeReader`, and `NewSectionReader`;
- the types `LimitedReader` and `SectionReader`.

`EOF` is a macro of `<cstdio>`, so `io.EOF` is `io::EOF_`, as the builtin `delete` is `eo::delete_`: `if err == io.EOF` is `if (err == io::EOF_)`. The errors are package-level `error` values, so they compare as Go's do, and `errors.Is` finds them in a wrapped error.

The interfaces are [interface types](#interface-types). A method of an interface value is called as a member, `r.Read(p)`, and the methods have Go's parameters and results: `Read(p []byte) (n int, err error)` is `auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>`, and `whence` and the other `int`s are `int64_t`. A translated type with the methods converts to the interface as any other does, and so do `ptr<bytes::Buffer>`, `ptr<bytes::Reader>`, and `ptr<strings::Reader>`.

`LimitedReader` and `SectionReader` are written as translated types: their methods are static members whose first parameter is the receiver, called as in [Methods](#methods). `lr := &io.LimitedReader{R: r, N: 4}` is `auto lr = New<io::LimitedReader>(io::LimitedReader{.R = r, .N = 4});`, `lr.Read(p)` is `io::LimitedReader::Read(lr, p)`, and `s.Size()` of a `*io.SectionReader` is `io::SectionReader::Size(s)`. `MultiReader` and `MultiWriter` are variadic, so their parameter is a slice: `io.MultiReader(a, b)` is `io::MultiReader(slice<io::Reader>::of({a, b}))`.

The functions are ported from Go 1.26's `io.go` and `multi.go`:

- `Copy` and `CopyN` use the source's `WriteTo` or the destination's `ReadFrom` when it has one, and otherwise copy through a buffer of 32 KiB, or of the remaining count of a `*LimitedReader` when that is smaller, and of one byte when that count is below 1. A short write gives `ErrShortWrite`, and a write that reports more bytes than it got gives `invalid write result`.
- `CopyBuffer` panics with `empty buffer in CopyBuffer` for a buffer that is empty and not nil.
- `ReadAll` grows its buffer as Go's does. Go rounds each capacity up to an allocation size class, and Eo does not, so the capacity of the result is outside this mapping.
- `MultiReader` flattens nested `MultiReader`s when it reads and when it writes to a `WriterTo`, and `MultiWriter` flattens nested `MultiWriter`s, as Go's do.
- `NopCloser` keeps the `WriterTo` of its argument, as Go's does.
- A method of a nil `Reader` or `Writer` panics with `runtime error: invalid memory address or nil pointer dereference`.

#### The io package not covered yet

- `Pipe`, `PipeReader`, and `PipeWriter`, `OffsetWriter` and `NewOffsetWriter`, and the `io/fs` and `io/ioutil` packages.

### Buffers and readers

`bytes.Buffer`, `bytes.Reader`, and `strings.Reader` map to `bytes::Buffer`, `bytes::Reader`, and `strings::Reader`, from `<eo/bytes.h>` and `<eo/strings.h>`, which include `<eo/io.h>`. Their methods have pointer receivers that put them in `io` interfaces, so, unlike `strings::Builder`, they are written as translated types, as `big::Int` is: each method is a static member whose first parameter is the receiver, and a call follows the call table in [Methods](#methods).

```go
var buf bytes.Buffer
buf.WriteString("hello")
r := strings.NewReader("abc")
io.Copy(&buf, r)
fmt.Println(buf.String(), r.Len())
```

```cpp
auto buf = New<bytes::Buffer>();
bytes::Buffer::WriteString(buf, "hello"s);
auto r = strings::NewReader("abc"s);
io::Copy(buf, r);
auto _eo_call_0 = bytes::Buffer::String(buf);
auto _eo_call_1 = strings::Reader::Len(r);
fmt::Println(_eo_call_0, _eo_call_1);
```

`&buf` converted to `io.Writer` moves `buf` to heap, so it is [boxed](#boxed-variables) and `&buf` is the box. A `bytes.Buffer` that is not boxed is `bytes::Buffer buf{};`, and `buf.Len()` is `bytes::Buffer::Len(ref(buf))`. `bytes.NewBuffer`, `bytes.NewBufferString`, `bytes.NewReader`, and `strings.NewReader` return a `ptr`. `fmt.Println(&buf)` prints the contents, through `String`, and `String` of a nil `*bytes.Buffer` is `<nil>`, as in Go.

The types have all their Go methods:

- `Buffer`: `Bytes`, `AvailableBuffer`, `String`, `Peek`, `Len`, `Cap`, `Available`, `Truncate`, `Reset`, `Grow`, `Write`, `WriteString`, `WriteByte`, `WriteRune`, `ReadFrom`, `WriteTo`, `Read`, `Next`, `ReadByte`, `ReadRune`, `UnreadRune`, `UnreadByte`, `ReadBytes`, and `ReadString`, with `MinRead` and `ErrTooLarge`;
- `bytes.Reader` and `strings.Reader`: `Len`, `Size`, `Read`, `ReadAt`, `ReadByte`, `UnreadByte`, `ReadRune`, `UnreadRune`, `Seek`, `WriteTo`, and `Reset`.

They are ported from Go 1.26's `buffer.go` and `reader.go`:

- `Bytes`, `Next`, `Peek`, and the slices that `ReadBytes` reads from alias the buffer until its next change, as in Go.
- `Truncate` panics with `bytes.Buffer: truncation out of range`, `Grow` with `bytes.Buffer.Grow: negative count`, and a buffer that cannot grow with `ErrTooLarge`.
- The errors of `UnreadByte`, `UnreadRune`, `ReadAt`, and `Seek` have Go's messages, such as `bytes.Reader.Seek: negative position` and `strings.Reader.UnreadByte: at beginning of string`.
- A `Buffer` grows as Go's does, but Go rounds each new capacity up to an allocation size class, and Eo does not. The values of `Cap` and `Available`, and the length of `AvailableBuffer`, after the buffer has grown are outside this mapping, as is whether a `Bytes` result still aliases the buffer after a write that grows it.
- Copying a `Buffer` copies its fields, so the copy shares the backing array, as in Go, where `go vet` reports the copy.

### The bufio package

Go's `bufio` maps to the namespace `bufio` from `<eo/bufio.h>`. It covers the whole package:

- the types `Reader`, `Writer`, `ReadWriter`, and `Scanner`, with all their methods, and `NewReader`, `NewReaderSize`, `NewWriter`, `NewWriterSize`, `NewReadWriter`, and `NewScanner`;
- `SplitFunc` and the split functions `ScanLines`, `ScanWords`, `ScanRunes`, and `ScanBytes`;
- the errors `ErrInvalidUnreadByte`, `ErrInvalidUnreadRune`, `ErrBufferFull`, `ErrNegativeCount`, `ErrTooLong`, `ErrNegativeAdvance`, `ErrAdvanceTooFar`, `ErrBadReadCount`, and `ErrFinalToken`, and the constant `MaxScanTokenSize`.

The types are written as translated types, as in [Buffers and readers](#buffers-and-readers): each method is a static member whose first parameter is the receiver. A `ptr<bufio::Reader>` is an `io::Reader`, `io::ByteScanner`, `io::RuneScanner`, and `io::WriterTo`, and a `ptr<bufio::Writer>` an `io::Writer`, `io::ByteWriter`, `io::StringWriter`, and `io::ReaderFrom`, as in Go:

```go
br := bufio.NewReader(r)
b, err := br.Peek(1)
w := bufio.NewWriter(&out)
fmt.Fprintf(w, "%d\n", len(b))
w.Flush()
```

```cpp
auto br = bufio::NewReader(r);
auto [b, err] = bufio::Reader::Peek(br, 1);
auto w = bufio::NewWriter(out);
fmt::Fprintf(w, "%d\n"s, eo::len(b));
bufio::Writer::Flush(w);
```

`ReadWriter` embeds `*Reader` and `*Writer`, so its fields are `Reader` and `Writer`, and `rw.ReadString('\n')` is `bufio::Reader::ReadString(rw->Reader, 10)`, as in [Embedded fields](#embedded-fields). Its method table has the forwarders of the promoted methods, without `Buffered`, `Reset`, and `Size`, which both embedded types declare, so Go leaves them out of its method set. `SplitFunc` is an alias of its function type, `fn<std::tuple<int64_t, slice<uint8_t>, error>(slice<uint8_t>, bool)>`, so a function literal or one of the split functions is passed to `Split` as it is. `Scanner` holds a function value, so it has no `operator==`, as a Go struct with a func field is not comparable.

They are ported from Go 1.26's `bufio.go` and `scan.go`:

- `Peek`, `ReadSlice`, `ReadLine`, `Bytes`, and the token that a split function returns alias the buffer until the next read, as in Go.
- `NewReaderSize` and `NewWriterSize` return their argument when it is a `*Reader` or `*Writer` whose buffer is large enough, and `Reset` of a reader or writer to itself does nothing.
- A `Reader` reads straight into a `Read` buffer at least as large as its own, and a `Writer` writes straight from a `Write` buffer larger than its free space when it holds nothing. `WriteString` then uses the underlying `io.StringWriter`, and `ReadFrom` the underlying `io.ReaderFrom`.
- The panics have Go's messages, such as `bufio: tried to fill full buffer`, `Split called after Scan`, and `bufio.Scan: too many empty tokens without progressing`, and a `Read` that reports a negative count panics with `bufio: reader returned negative count from Read`.

#### The bufio package not covered yet

- `%T` of a `SplitFunc` value, which prints its function type, where Go prints `bufio.SplitFunc`.

### The reflect package

Go's `reflect` maps to the namespace `reflect` from `<eo/reflect.h>`, ported from Go 1.26's `reflect` on the type information that Eo keeps for every type an interface can hold, as Go's runtime does. It covers:

- `Kind`, a defined type with its `String` method, and its constants, `reflect::Invalid` through `reflect::UnsafePointer`, and `reflect::Ptr`;
- `Type`, with the methods `Kind`, `Name`, `String`, `Elem`, `Key`, `Len`, `NumField`, `Field`, `NumMethod`, `Implements`, `AssignableTo`, `Comparable`, and `Bits`, and `StructField`, with `IsExported`, and `StructTag`, with `Get` and `Lookup`;
- `Value`, with the methods `Kind`, `Type`, `IsValid`, `CanAddr`, `CanSet`, `CanInterface`, `Interface`, `IsNil`, `IsZero`, `Elem`, `Addr`, `NumField`, `Field`, `FieldByIndex`, `Len`, `Cap`, `Index`, `Bool`, `Int`, `Uint`, `Float`, `String`, `Bytes`, `Set`, `SetBool`, `SetInt`, `SetUint`, `SetFloat`, `SetString`, `SetBytes`, `SetZero`, `SetLen`, `Grow`, `MapIndex`, `MapKeys`, `MapRange`, `SetMapIndex`, `NumMethod`, `OverflowInt`, `OverflowUint`, and `OverflowFloat`, and `MapIter`, with `Next`, `Key`, and `Value`;
- the functions `TypeOf`, `TypeFor`, `PointerTo`, `PtrTo`, `ValueOf`, `New`, `Zero`, `Indirect`, `MakeSlice`, `MakeMap`, `MakeMapWithSize`, `Copy`, and `TypeAssert`, and `ValueError`, which the `Value` methods panic with.

`Value`, `StructField`, `StructTag`, `ValueError`, and `MapIter` are written as translated types, as in [Buffers and readers](#buffers-and-readers): each method is a static member whose first parameter is the receiver, so `v.Field(0).Int()` is `reflect::Value::Int(reflect::Value::Field(v, 0))`, and `MapRange` returns a `ptr<reflect::MapIter>`. Go's `Type` is an interface that only the runtime's type implements, so it maps to one class, `reflect::Type`, whose methods are called as members, as a translated interface's are: `t.Elem().Kind()` is `t.Elem().Kind()`. A nil `Type` is `nullptr`, two `Type`s are `==` when they are the same type, and a `Type` prints by its `String` method. The generic functions take their type argument as a template argument: `reflect.TypeFor[error]()` is `reflect::TypeFor<error>()`, and `x, ok := reflect.TypeAssert[int](v)` is `auto [x, ok] = reflect::TypeAssert<int64_t>(v);`.

```go
v := reflect.ValueOf(&p).Elem() // p is a Point
for i := 0; i < v.NumField(); i++ {
	f := v.Type().Field(i)
	fmt.Println(f.Name, f.Tag.Get("json"), v.Field(i).Interface())
}
v.Field(0).SetInt(7)
```

```cpp
auto v = reflect::Value::Elem(reflect::ValueOf(p)); // p is boxed: auto p = New<Point>();
for (auto i = int64_t{0}; i < reflect::Value::NumField(v); i++) {
  auto f = reflect::Value::Type(v).Field(i);
  auto _eo_call_0 = reflect::StructTag::Get(f.Tag, "json"s);
  auto _eo_call_1 = reflect::Value::Field(v, i);
  auto _eo_call_2 = reflect::Value::Interface(_eo_call_1);
  fmt::Println(f.Name, _eo_call_0, _eo_call_2);
}
reflect::Value::SetInt(reflect::Value::Field(v, 0), 7);
```

A `Value` holds the address of the value's storage, shared with what owns it, as a Go `Value` points into the heap, so setting a field through `reflect::ValueOf(p)` changes `*p`. The rules of Go's `reflect` hold:

- A `Value` from `ValueOf` cannot be set. `Elem` of a pointer, `Index` of a slice, and `Field` or `Index` of a value that can be set give values that can be set, unless they come through an unexported field. The exported fields of an unexported embedded struct can still be set, as in Go, which `encoding/json` relies on to decode them. A field is exported when its name starts with an ASCII upper-case letter, as in [Printing methods](#printing-methods).
- `Interface` copies a value that can be addressed and shares one that cannot, which no one can change.
- `Set` assigns a value of the same type, of the same underlying type when one of the two types is unnamed, as a `[]byte` to a `Bytes`, or of a type that has every method of the interface it is assigned to.
- `Field` reports the tags and embedded fields of a struct's field table (see [Field tags and embedding](#field-tags-and-embedding)). `PkgPath` of an unexported field is its package's name, which Eo's type names keep, rather than its import path, and `Offset` is the C++ offset, which differs from Go's.
- `IsZero` follows Go 1.26: a float is zero when it equals 0, so `-0.0` is zero and NaN is not.
- `MapRange` iterates over a copy of the map's entries, taken by the first `Next`, and `MapKeys` returns the keys in the map's iteration order, which Go does not specify.
- `Grow` picks a capacity by the formula of `append` (see [Slices](#slices)), without Go's rounding to an allocation size.
- The panics have Go's messages and values: a `*reflect.ValueError`, such as `reflect: call of reflect.Value.Int on string Value`, or a string, such as `reflect: reflect.Value.SetInt using unaddressable value`.

Go's `int` and `int64` are both `int64_t`, and `uint` and `uint64` both `uint64_t` (see [Basic types](#basic-types)), so `reflect` reports the kinds `Int` and `Uint` and the names `int` and `uint` for both, as `%T` does.

#### The reflect package not covered yet

- the kinds `Int64` and `Uint64`, which Eo reports as `Int` and `Uint`, and `Uintptr`, `Complex64`, `Complex128`, and `UnsafePointer`;
- methods and calls: `Type.Method`, `Value.Method`, `MethodByName`, `Call`, and `MakeFunc`, and conversions: `Convert`, `CanConvert`, and `ConvertibleTo`;
- `DeepEqual`, `Append`, `AppendSlice`, `Value.Equal`, `Value.Comparable`, `Value.Clear`, `Value.FieldByName`, `Value.FieldByNameFunc`, `Value.CanInt`, `Value.CanUint`, `Value.CanFloat`, `Value.CanComplex`, `Value.Complex`, `Value.SetComplex`, `Value.OverflowComplex`, `Value.SetCap`, `Value.Slice`, `Value.Slice3`, `Value.Pointer`, `Value.UnsafePointer`, `Value.UnsafeAddr`, `Value.Seq`, and `Value.Seq2`, and `MapIter.Reset`, `Value.SetIterKey`, and `Value.SetIterValue`;
- `Type.FieldByName`, `Type.FieldByNameFunc`, `Type.FieldByIndex`, `Type.OverflowInt`, `Type.OverflowUint`, `Type.OverflowFloat`, `Type.OverflowComplex`, `Type.PkgPath`, `Type.Size`, `Type.Align`, the function type methods `In`, `Out`, `NumIn`, `NumOut`, and `IsVariadic`, and `VisibleFields`;
- channels: `Len` and `Cap` of a channel `Value`, `Type.ChanDir`, `Send`, `Recv`, `Close`, `MakeChan`, and `Select`, and the constructors of types: `SliceOf`, `MapOf`, `ArrayOf`, `StructOf`, `FuncOf`, and `ChanOf`;
- `PointerTo` of a pointer to a pointer, and `Interface` of a function or channel value;
- `Interface` of a `*T` that `New`, `Addr`, or a `Value` of `PointerTo(t)` holds, where `T` has no methods and the program names `*T` nowhere else, such as in a conversion to an interface, a type assertion, or a field type;
- `Bytes` and `SetBytes` of a slice or array whose element is a defined byte type;
- a struct type declared inside a function, which has no field table;
- printing a `Value` inside another value, which Go prints by the `Value`'s own fields. A `Value` operand prints the value it holds, as in Go.

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
- translate addresses, pointers, and captures as described in [Pointers and escaping variables](#pointers-and-escaping-variables), which boxes every variable that a goroutine can reach, so that no launch needs a lifetime check of its own (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary)); a launch that passes a value referring to storage through a type whose mapping is not frozen, such as a string view, is provisional;
- call a method of a translated type as `T::M(receiver, args...)` with the saved receiver;
- apply Go 1.22 and later per-iteration loop-variable semantics; a loop's own post statement or `range` update is not a write to the variables that its clause declares with `:=`, but it is a write to any other variable.

`go(...)` returns without running the launched call, including when it is called from a running goroutine.

The launched function body begins only after the source `go` statement's function and argument evaluation is complete.

### Channels

Go `chan T` maps to `chan<T>` from `<eo/chan.h>`, which `<eo/core.h>` includes. A `chan<T>` is a handle to a shared channel, so copies, parameters, and captures refer to the same channel, as in Go. Channel operations block only the goroutine (see [Blocking operations](#blocking-operations)).

| Go | C++ |
| --- | --- |
| `chan T`, `<-chan T`, `chan<- T` | `chan<T>` |
| `nil` as a `chan T`, and the zero value of `chan T` | `chan<T>{}`, or `nullptr` where it converts; `var ch chan T` is `chan<T> ch;` |
| `ch == nil`, `ch != nil`, `ch1 == ch2` | `ch == nullptr`, `ch != nullptr`, `ch1 == ch2` |
| `make(chan T)`, `make(chan T, n)` | `eo::make_chan<T>()`, `eo::make_chan<T>(n)` |
| `ch <- v` | `co_await (ch << v);` |
| `<-ch` as a value, as in `v := <-ch` | `co_await *ch`, as in `auto v = co_await *ch;` |
| `<-ch` as a statement | `co_await *ch;` |
| `v, ok := <-ch` | `auto [v, ok] = co_await eo::recv_ok(ch);` |
| `v, ok = <-ch` | `std::tie(v, ok) = co_await eo::recv_ok(ch);` |
| `close(ch)` | `ch.close();` |
| `cap(ch)` | `eo::cap(ch)`, as `int64_t` |
| `chan struct{}` | `chan<empty_struct>` |

A channel's direction is checked by the Go compiler, so a translation drops it: `<-chan T` and `chan<- T` are `chan<T>`. `eo::make_chan` and `eo::recv_ok` are function objects, called qualified, like the string helpers (see [Strings](#strings)). The size argument of `eo::make_chan` accepts any Go integer type. A negative size, or one whose buffer would be larger than Go's allocation limit, panics with `makechan: size out of range`, a `runtime.plainError`, as in Go.

`co_await *ch` gives the received value, or the zero value once `ch` is closed and its buffer is drained. `eo::recv_ok` gives a `std::pair` of the value and `true`, or of the zero value and `false` once `ch` is closed and drained, so a zero value that was sent is told apart from a close. It follows the forms of [Multiple results](#multiple-results) with a `std::pair`, as `eo::type_assert_ok` does: `_, ok := <-ch` is `auto ok = (co_await eo::recv_ok(ch)).second;`. The parentheses are needed, because `.` binds more tightly than `co_await`.

Values buffered before `close` are still received, as in Go. A send on a closed channel, including a send that is blocked when the channel is closed, panics with `send on closed channel`. `close` of a nil channel panics with `close of nil channel`, and of a closed one with `close of closed channel`. All three panic values are `runtime.plainError`. Sending to and receiving from a nil channel blocks forever.

`co_await (ch << v)` converts `v` to `T` when the send runs. A send statement and a receive in an expression follow [Evaluation order](#evaluation-order), where a receive is an ordered operation. A `select` statement uses `*ch` and `ch << v` as its cases (see [Select](#select)). `for range` over a channel is in [Range loops](#range-loops).

#### Channels not covered yet

- `len(ch)`, the number of buffered values;
- printing a channel with `fmt`, holding one in an interface, and `%T` of a channel type;
- a channel of channels with a direction, such as `chan<- <-chan int`, whose `%T` would differ; the operations on it are covered.

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

- Each result has storage declared before `eo_defer_scope`: a named result uses its translated name, and an unnamed result `N` uses `_eo_result_N`. Both are declared as `T name{}` with the translated result type, one declaration per result.
- `try {` opens immediately before the first statement at the top level of the function body that is, or contains, a `defer` statement, and before that statement's argument temporaries. It closes at the end of the function body. A `goto` that jumps into it moves it earlier (see [Goto](#goto)). A panic before that point has no deferred call to run and leaves the function directly. A `defer` inside a nested function literal belongs to that literal, which gets this shape itself, and does not count for the enclosing function.
- After the catch handler, the function returns its result storage: `return _eo_result_0;`, `co_return _eo_result_0;`, or the named results, and `return {_eo_result_0, _eo_result_1};` for several results (see [Multiple results](#multiple-results)). A function without results ends with nothing, or with `co_return;` for a coroutine. This return is reached only when a deferred call recovered the panic.

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

If the deferred callee is itself a function-valued expression, such as a call that returns a function or a variable of function type, evaluate and save it before the arguments:

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

With several results, the return assigns all of the storage at once, with the `std::tie` form of [Multiple results](#multiple-results), and returns it as a tuple:

```go
return y, x // named results x and y
return g()  // g has two results
```

```cpp
std::tie(x, y) = std::tuple<int64_t, int64_t>{y, x};
eo_defer_run;
return {x, y};

std::tie(_eo_result_0, _eo_result_1) = g();
eo_defer_run;
return {_eo_result_0, _eo_result_1};
```

The braced list evaluates every operand before any result is assigned, as Go does. Assigning the results one by one would make `return y, x` return `y` twice, and a panic in the second operand would leave the first result assigned, which a deferred `recover()` would then return.

Coroutine returns follow the same rule:

```cpp
eo_defer_run;
co_return;
```

A function that reaches its end without an explicit return inserts `eo_defer_run` at the natural function exit. Every early normal return must have its own corresponding drain sequence.

### Panic and recover

`panic(v)` translates to `panic(v)` from `<eo/panic.h>`, which throws `eo::Panic`. It converts the value to `any`, as Go's `panic` takes `any`: a string literal becomes a `std::string`, and an interface value passes its dynamic value. `value()` returns the `any`, and `message()` the value as `%v` prints it, which calls its `Error` or `String` method (see [Formatted printing](#formatted-printing)). `what()` is `panic: ` followed by the message. `panic(nil)` translates to `panic(nullptr)`, and it and `panic` of a nil interface value raise a `*runtime.PanicNilError`, whose message is `panic called with nil argument`, as since Go 1.21.

The panics that Eo raises for Go's run-time errors have Go's value types:

| Panic | Value type | Message |
| --- | --- | --- |
| a nil dereference, an integer division by zero, a negative shift count, a `make` length out of range, a comparison of an uncomparable type | `runtime.errorString` | `runtime error: invalid memory address or nil pointer dereference` |
| an index or a slice bound out of range | `runtime.boundsError` | `runtime error: index out of range [3] with length 0` |
| a failed type assertion or interface conversion | `*runtime.TypeAssertionError` | `interface conversion: interface {} is string, not int` |
| a close of a nil or closed channel, a send on a closed channel, and `make(chan T, n)` with a size out of range | `runtime.plainError` | `close of nil channel` |
| `panic(nil)` | `*runtime.PanicNilError` | `panic called with nil argument` |

Each of these is an `error` and a `runtime.Error`, which is `runtime::Error_` (see [Types named like one of their methods](#types-named-like-one-of-their-methods)), so `r.(runtime.Error)` is `eo::type_assert_ok<runtime::Error_>(r)`. The values keep Go's message, not Go's fields. The library panics with a Go string where Go does: `sync: negative WaitGroup counter`, `cannot create context from nil parent`, and `invalid argument to Intn`. Any other C++ exception that leaves a translated call is also a panic, and `recover()` returns it as an `error` whose dynamic type is `eo.cppException` and whose message is its `what()`.

While a panic leaves a function that contains a `defer` statement, its catch handler runs `eo_defer_panic`:

- the remaining deferred calls run in LIFO order, after those of the functions it called and before those of its callers;
- a deferred call that panics replaces the running panic, and the remaining deferred calls still run;
- if the last panic was recovered, the function returns its result storage normally, and its caller continues. Otherwise the panic leaves the function.

A panic in a deferred call during `eo_defer_run` leaves `eo_defer_run`, and the same catch handler runs the remaining deferred calls.

`recover()` translates to `recover()`, which returns the panic value as an `any`, as in Go, so `r` is an ordinary [interface value](#interface-values):

- `r := recover()` translates to `auto r = recover();`;
- `r == nil` and `r != nil` translate to `r == nullptr` and `r != nullptr`;
- `err, ok := r.(error)` translates to `auto [err, ok] = eo::type_assert_ok<error>(r);`, and the other assertions and type switches follow [Type assertions](#type-assertions) and [Type switches](#type-switches);
- printing `r` prints the panic value as Go does, and `<nil>` for nil;
- `panic(r)` starts a new panic with the recovered value, and `panic(r)` with a nil `r` is `panic(nil)`.

`recover()` returns the running panic, and stops it, only when it is called by a deferred call that `eo_defer_panic` runs. It returns nil when no panic is running, in a deferred call run by `eo_defer_run`, and after the panic was recovered. Go also returns nil when recover is not called directly by the deferred function. Eo cannot detect that. A `recover()` is frozen when it is in one of these places:

- directly in the body of a deferred function literal, `defer func() { ... recover() ... }()`, but not in a function literal nested in that body;
- directly in the body of a named function or method that the program calls only in `defer` statements.

Any other `recover()` is unsupported, because a call from inside a deferred function would recover where Go returns nil.

The report of a panic that no goroutine recovers prints the value as `%v` does. Go prints a value that is not an error, a `Stringer`, or of a basic kind as its type and address, such as `(main.T) 0xc000012345`, and adds ` [recovered, repanicked]` for a value raised again. Stderr is not part of parity. `runtime.Goexit` has no mapping.

### Blocking operations

A Go operation that blocks the calling goroutine translates to an awaitable that the translation suspends on with `co_await`. It never blocks the OS thread. A blocked worker thread stops other goroutines from running, and with `EOMAXPROCS=1` it stops all of them, while Go parks only the goroutine.

| Go | C++ | Status |
| --- | --- | --- |
| `select` without `default` | `co_await select.index()` | frozen, see [Select](#select) |
| `wg.Wait()` | `co_await wg.Wait()` | frozen |
| `mu.Lock()` | `co_await mu.Lock()` | frozen, see [Mutex](#mutex) |
| `rw.Lock()`, `rw.RLock()` | `co_await rw.Lock()`, `co_await rw.RLock()` | frozen, see [Mutex](#mutex) |
| `ch <- v` | `co_await (ch << v)` | frozen, see [Channels](#channels) |
| `v := <-ch`, `v, ok := <-ch` | `auto v = co_await *ch`, `auto [v, ok] = co_await eo::recv_ok(ch)` | frozen, see [Channels](#channels) |
| `for v := range ch` | a loop on `co_await eo::recv_ok(...)` | frozen, see [Range loops](#range-loops) |
| `time.Sleep(d)` | `co_await time::Sleep(d)` | provisional, including the duration |

A Go function whose body contains a blocking operation, or a call to a function translated as a coroutine, is translated as a coroutine returning `func<T>`, and every call to it is `co_await`ed. Other functions stay plain C++ functions. Calls through function values follow [Coroutine function values](#coroutine-function-values). Calls through interface methods, whose target is not known statically, are provisional ([#108](https://github.com/conr2d/eo/issues/108)).

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

- a Mutex or RWMutex as a struct field. A package-level one follows [Package-level variables](#package-level-variables), as in `tests/parity/mutex_counter`, and sharing a local one follows the WaitGroup rules above, as in `tests/parity/mutex_local_shared`;
- `RLocker`, and passing a lock as a `sync.Locker`, have no mapping;
- `sync::Mutex`, `sync::RWMutex`, and `sync::WaitGroup` cannot be copied or moved. A Go program that copies one, which `go vet` reports, is unsupported.

### Package-level variables

Go initializes package-level variables in dependency order and then runs the `init` functions, all in the main goroutine before `main`. C++ runs the dynamic initializers of namespace-scope variables before `main` too, but in declaration order, and outside the Eo runtime, where a goroutine, a blocking operation, or a panic cannot work. So a package-level variable is declared at namespace scope with nothing that C++ has to run in order, and its initializer runs in a generated function, `eo_init`, which the runtime calls in the main goroutine before `eo_main`.

#### Declaring package-level variables

Each package-level variable is one C++ declaration at namespace scope, in source order, and a `var` group or a declaration of several names is one declaration per name. The storage lasts for the whole program, so a package-level variable is never [boxed](#boxed-variables), `&x` is `ref(x)` (see [Pointer type](#pointer-type)), and function literals use it without capturing it (see [Function literals](#function-literals)).

- A variable without an initializer, or with a constant initializer (one for which `go/types` records a constant value), is declared as the same `var` declaration in a function body is, without boxing: `var count int` is `int64_t count{};`, `var limit = 3` is `auto limit = int64_t{3};`, and `var name = "eo"` is `auto name = "eo"s;` (see [Untyped constants](#untyped-constants)). `var mu sync.Mutex`, `var rw sync.RWMutex`, and `var wg sync.WaitGroup` are `sync::Mutex mu;`, `sync::RWMutex rw;`, and `sync::WaitGroup wg;`. Such an initializer reads no variable and has no effect, so running it before `main` is the same as running it in Go's order. A struct composite literal with no elements, `T{}`, is the zero value and counts as no initializer, so `var mu = sync.Mutex{}` is `sync::Mutex mu;` too: C++ cannot assign a `sync::Mutex`. A slice or map literal is not the zero value, because it is never nil.
- Any other initializer is a *dynamic initializer*. Its variable is declared with the zero value of its type, written out, as `T x{};`, or `slice<T> s;`, `map<K, V> m;`, and `chan<T> ch;` for the types whose tables give that form. Its initializer is an assignment in `eo_init`. `var ErrEmpty = errors.New("empty")` is `error ErrEmpty{};`, `var twice = func(n int) int { return n * 2 }` is `fn<int64_t(int64_t)> twice{};`, and `var p = &count` is `ptr<int64_t> p{};`.
- The blank identifier `_` declares nothing.

A comment on a variable declaration moves with its declaration.

#### Package initialization

In a program of one file, `func<> eo_init()` is defined after every other declaration of the file. A program of several files gives each package its own `eo_init` with the same body (see [Initialization of packages](#initialization-of-packages)). Its body has, in order:

1. one statement for each entry of `go/types` `Info.InitOrder` whose initializer is a dynamic initializer, in that order. A constant and an empty struct literal `T{}` get no statement. The entry `x = e` is the translation of the assignment `x = e` in a function body, which follows [Evaluation order](#evaluation-order), `co_await`s a coroutine call (see [Blocking operations](#blocking-operations)), and writes a function literal as a lambda. An entry with several variables, as in `var q, r = divmod(17, 5)`, is the assignment `q, r = divmod(17, 5)`, which takes the first form of [Declaring and assigning several results](#declaring-and-assigning-several-results) that applies to an assignment: `std::tie(q, r) = divmod(17, 5);`, and `var _, w = f()` is `w = std::get<1>(f());`. A declaration with one initializer per variable, as in `var u, v = trace("u", 4), trace("v", u+1)`, has one entry for each variable, so it is `u = trace("u"s, 4);` and then `v = trace("v"s, u + 1);`, each at its own place in `Info.InitOrder`. Unlike the assignment `u, v = ...` in a function body, the second initializer sees the first variable's new value, as in Go;
2. a call of each `init` function, in source order: `_eo_init_0();`, or `co_await _eo_init_0();` for a coroutine;
3. `co_return;`.

`eo_init` is one function for the temporaries `_eo_call_<n>` of [Saving ordered operations](#saving-ordered-operations), so they are numbered from 0 across all its statements.

A blank variable's dynamic initializer is still evaluated, at its place in `Info.InitOrder`. Where the declaration names a type `T`, the statement is `static_cast<void>(T(e));`, so `var _ fmt.Stringer = (*Box)(nil)`, Go's compile-time check that `*Box` implements `fmt.Stringer`, is `static_cast<void>(fmt::Stringer(ptr<Box>{}));`, which C++ checks too. Otherwise the statement is the expression statement that the initializer is in a function body: a call is the call statement, as `var _ = f()` is `f();`, or `co_await f();` when `f` is a coroutine, and a receive `var _ = <-ch` is `co_await *ch;`. Any other initializer is `static_cast<void>(e);`. In every form, `e` is the translated initializer, so a coroutine call inside it is `co_await`ed, as in `static_cast<void>(int64_t(co_await f()));`. The comment on a blank variable's declaration moves with its statement.

Each `func init()` is a function named `_eo_init_<n>`, where `n` is its position among the file's `init` functions, from 0, or among the package's in a package of several files. It is defined where Go declares it, as `void _eo_init_0()`, or as `func<> _eo_init_0()` when it is a coroutine, by the same rule as any other function. Go code cannot call or refer to an `init` function, so nothing else names it. A Go package-level name `eo_init` or `eo_main` collides with these functions and is not covered yet ([#83](https://github.com/conr2d/eo/issues/83)).

A file with no `init` function and no dynamic initializer defines no `eo_init`. `eo::main` provides an empty one, as a weak definition, in its place.

```go
var total = sum(first, second)
var first = trace("first", 1)
var second = 2
var ErrEmpty = errors.New("empty")

func init() {
	fmt.Println("init:", total)
}
```

```cpp
int64_t total{};
int64_t first{};
auto second = int64_t{2};
error ErrEmpty{};

// sum and trace

void _eo_init_0() {
  fmt::Println("init:"s, total);
}

// main

func<> eo_init() {
  first = trace("first"s, 1);
  total = sum(first, second);
  ErrEmpty = errors::New("empty"s);
  _eo_init_0();
  co_return;
}
```

`eo::main` runs `co_await eo_init();` and then `co_await eo_main();` as one coroutine, the main goroutine. So an initializer or an `init` function can block, and a goroutine that it starts runs while initialization continues, as in Go. A panic that escapes `eo_init` is an unrecovered panic of the main goroutine: `eo_main` does not run, and the process exits with status 2 (see [Program termination](#program-termination)).

#### Package-level variables not covered yet

- a dynamic initializer of a struct type that holds a `sync.Mutex`, `sync.RWMutex`, or `sync.WaitGroup`, as in `var c = Cache{items: make(map[string]int)}`. C++ cannot assign such a struct, and a lock as a struct field is not frozen (see [Mutex](#mutex));
- an initializer or `init` function that blocks forever: Go reports a deadlock and exits with status 2, and Eo hangs, as it does when every goroutine blocks during `main`.

### Packages

A program whose only package is `main`, in one file, translates to one C++ file whose declarations are in the global namespace, as the rest of these rules show. Every other program translates each of its packages, including `main`, to a directory of C++ files in a namespace of its own. These rules apply to every package that the program imports, except the standard packages that Eo maps, such as `fmt`, `errors`, `sync`, and `maps`, whose calls follow their own sections. A test package, a `_test.go` file, and a package that uses `cgo` are not covered.

#### Package files

A package with the import path `P` translates to a directory with:

- `package.h`, the package header, which declares everything that the package declares at package level;
- one C++ file `F.cpp` for each Go file `F.go` that the go command builds for the package, which `go list` reports as `GoFiles`. It holds the definitions of the functions and methods that `F.go` declares.

A file includes a package header by its import path, as `#include "P/package.h"`, so the build maps each import path to its directory. The parity harness maps the import path of a case to its `cpp` directory (see [tests/parity](../tests/parity/README.md#several-files)). Go presents the files of a package to the compiler sorted by name, and *file order* below means that order.

The package header has, in order:

1. `#pragma once`;
2. the Eo headers, then the C++ standard headers, that any file of the package needs, each group sorted by name, then the headers of the packages that any file imports, sorted by import path, including a blank import `_`;
3. `using namespace eo;`;
4. the package namespace (see [Package namespaces](#package-namespaces)), which holds the rest;
5. the namespace aliases of [Import names](#import-names) that the header declares, sorted by the import path they alias and then by name;
6. the package-level constants, then the declarations of [Declaration order](#declaration-order) steps 1 to 3, over all the package's files, each group taken in file order and then in source order. Step 2 applies across files, so a struct that holds a type of another file by value is defined after that type;
7. a prototype of every function the package declares, including `init` functions and `eo_main`, in file order and then in source order. `eo_main` is always `func<> eo_main()`, because the global `eo_main` `co_await`s it. A function `main` of a package other than `main` keeps its name, which C++ allows in a namespace;
8. the package-level variables, in file order and then in source order;
9. the package's `eo_init` (see [Initialization of packages](#initialization-of-packages)).

A namespace-scope variable or constant in a package header is `inline`, so every file shares one object: `var count int` is `inline int64_t count{};`, `var name = "eo"` is `inline auto name = "eo"s;`, and `const KB = 1024` is `inline constexpr int64_t KB = 1024;`. Functions, methods, and types keep the forms that their sections give. A comment on a declaration moves with the declaration into the header.

A file `F.cpp` has, in order:

1. `#include "P/package.h"`, its only include, because the header includes what every file needs;
2. the package namespace, which holds the rest;
3. the namespace aliases of [Import names](#import-names) that this file declares, sorted by the import path they alias and then by name;
4. the definitions of [Declaration order](#declaration-order) step 4 for the types that `F.go` declares;
5. the definitions of the functions and methods that `F.go` declares, in source order. A method is defined in the file that declares it, which can differ from the file that declares its type.

A comment before the `package` clause, such as a package comment, comes first in `F.cpp`.

#### Package namespaces

The namespace of a package is its import path with every character that is not an ASCII letter or digit written `_`: the package `github.com/ethereum-optimism/optimism/op-node/rollup/derive` is in `namespace github_com_ethereum_optimism_optimism_op_node_rollup_derive`. The namespace is one identifier and not nested namespaces such as `github_com::ethereum_optimism::...::derive`. C++ looks up an unqualified name in the enclosing namespaces too, so in a nested `...::rollup::derive`, the name `sync` in `sync::Mutex` would find the sibling package `...::rollup::sync` before Eo's `sync`. A flat namespace has only the global namespace around it, which holds the other packages' namespaces and the program's `eo_init` and `eo_main`. Two import paths that give the same identifier, and an import path that gives an identifier that starts with `_` or a digit, holds `__`, or names a global that already exists, such as `eo`, `std`, `boost`, `main`, `eo_init`, or `eo_main`, are not covered.

The package header opens the package namespace with `namespace _eo_package = ::N;`, where `N` is the package's own namespace. The files reopen the namespace and use the alias that the header declares. A package-level name that a member hides is written `_eo_package::Name` in a package namespace, where a program of one file writes `::Name` (see [Package-level names hidden by members](#package-level-names-hidden-by-members)), so `A` embedded in a struct is `_eo_package::A A{};`.

A type's `_eo_name` keeps Go's name, the package name and the type name, and not the import path: `a.Point`, `util.Box`, and `main.Holder`. Two types of one name in two packages still differ in C++, so an interface tells them apart, although `%T` prints both alike, as Go does.

#### Import names

Go code names a package by its import name, the name in the import declaration or else the package name, in the scope of one file. A qualified identifier `p.Name` is `p::Name`, and `p` is a namespace alias, `namespace p = ::N;`, of the package's namespace `N`:

- When an import name names the same package in every file of the package that imports anything by that name, the package header declares its alias. A standard package that Eo maps counts as a package here, so a translated package `sync` in one file and the standard `sync` in another give file aliases. Go forbids a package-level name that is also an import name of any file of the package, even the same file, and reports it as `already declared through import of package`, so the alias never collides with a name of the package.
- Otherwise each file declares the alias for its own import, and the package header writes that package's names with its namespace, as `::N::Box`. A definition in `F.cpp` writes them with the file's alias, as `util::Box`.
- A standard package that Eo maps needs no alias when its import name is its package name, because `using namespace eo;` makes its namespace visible. Under another import name, as in `import f "fmt"`, its alias names Eo's namespace, `namespace f = eo::fmt;`, and is declared as an alias of a translated package would be, or at global scope after `using namespace eo;` in a program of one file. An alias of a translated package in the package namespace hides an Eo namespace of the same name, as a package `sync` of the program does.
- A blank import `_` declares no alias. Its header is still included, and its `eo_init` still runs.

A member hides an import name as it hides a package-level name, so inside a struct definition and the definitions of its methods, the import name of a translated package that is also the name of a member is written `_eo_package::p`, as in `_eo_package::trace::Add(...)` in a method of a struct with a field `trace`. A standard package that Eo maps and a member hides is written with `eo::` and the name of its Eo namespace, not its import name, as `eo::fmt` for `import f "fmt"` too, like any other Eo name that a member hides.

A dot import `.`, and an import name that is a C++ keyword, `eo`, `boost`, `eo_init`, `eo_main`, or an Eo name that translations use unqualified, such as `ptr` or `std`, are not covered ([#83](https://github.com/conr2d/eo/issues/83)).

#### Initialization of packages

Each package header defines the package's `inline func<> eo_init()`, even when it only `co_return`s. Its body is the body that [Package initialization](#package-initialization) gives, over the whole package: the entries of `go/types` `Info.InitOrder`, which covers every file of the package, then the calls of the `init` functions in file order and then in source order. The `n` of `_eo_init_<n>` counts the `init` functions of the package from 0 in that order.

The program's initialization and its `main` are the global functions `eo_init` and `eo_main`, which `eo::main` runs. They are defined at the end of the file that declares `func main()`, after its package namespace. The global `eo_init` `co_await`s the `eo_init` of every translated package, in Go's order, and the global `eo_main` `co_await`s the `eo_main` of package `main`:

```cpp
func<> eo_init() {
  co_await github_com_example_app_trace::eo_init();
  co_await github_com_example_app_m::eo_init();
  co_await github_com_example_app_z::eo_init();
  co_await github_com_example_app_a::eo_init();
  co_await github_com_example_app::eo_init();
}

func<> eo_main() {
  co_await github_com_example_app::eo_main();
}
```

Go's order is the one the Go specification gives since Go 1.21. Take the list of every package of the program, the standard packages included, sorted by import path. Repeatedly initialize the first package in the list that is not initialized and whose imports are all initialized. Package `main` imports, directly or not, every other package, so it comes last. The standard packages take part in the order, because they can delay a translated package that sorts before them, but they get no call: Eo initializes its own packages. This is not the order of a depth-first walk of the imports: a `main` that imports `a` and `m`, where `a` imports `z` and both `m` and `z` import `trace`, initializes `trace`, `m`, `z`, and `a`, in this order.

A package's `eo_init` does not call the `eo_init` of its imports, so each package is initialized once, however many packages import it.

#### Packages not covered yet

- the forms named in the sections above: test packages, `cgo`, dot imports, the import paths that give no valid namespace, and the import names of [#83](https://github.com/conr2d/eo/issues/83);
- the unexported methods of an interface that a type of another package implements: Go tells apart unexported method names of different packages, and `eo::method` names a method by its name alone;
- a package whose files depend on build constraints for platforms other than the one the translation targets.

### Program termination

A translated program links `eo::main` and defines `func<> eo_main()` in place of Go's `main`, which runs after package initialization (see [Package initialization](#package-initialization)). The runtime ends the process the way Go does:

- when `eo_main` completes, the process exits with status 0 at once, without waiting for other goroutines. If another goroutine has an `eo::Panic` that is not yet recovered, the runtime first waits up to about 100 ms for it, as Go's `runtime.main` yields to goroutines running deferred calls for a panic. Another C++ exception counts only while one function runs its deferred calls for it, not while it passes between functions. If a panic is then being reported, it ends the process with status 2 instead;
- `os.Exit(code)` translates to `os::Exit(code)` from `<eo/os.h>`. It ends the process at once from any goroutine, and deferred calls do not run. Do not translate it to `std::exit`: that runs static destructors, and the runtime's thread pool destructor then waits forever for its worker threads;
- an exception that escapes `eo_init` or `eo_main`, or the call launched by `go(...)`, is an unrecovered panic. The panicking goroutine's deferred calls have already run in its catch handlers (see [Panic and recover](#panic-and-recover)). The runtime reports the panic on stderr and exits with status 2;
- `fmt::Println`, `fmt::Printf`, and the other printing functions write and flush each call, as Go writes unbuffered to `os.Stdout` and `os.Stderr`. Output is not lost when the process is killed or exits.

Each exit flushes `std::cout`, `stdout` and `stderr`, then ends the process with `std::_Exit`. A stream whose lock another thread holds is skipped, for example while a goroutine is blocked writing to a pipe that nobody reads; that write is lost, as it would be in Go. Other `FILE` streams are not flushed, and no static destructors run.

With `EOMAXPROCS=1`, a goroutine blocked in a read or write occupies the only worker thread, so the goroutine that would call `os::Exit` or return from `eo_main` never runs. Go hands the processor to another thread during a blocking system call. See [#71](https://github.com/conr2d/eo/issues/71).

## Unfrozen constructs

If no canonical translation rule exists for a Go construct, do not silently invent a project-wide convention and treat it as stable.

A translator may use a local adaptation when necessary to make progress, but it should be explicit that the mapping is provisional and may need to change when the translation contract is extended.

Translation-facing mappings with broad source impact should be frozen before large-scale porting depends on them.

Examples currently requiring additional canonical rules include:

- goroutine launches that pass a non-owning view argument, or a value that refers to storage through a type whose mapping is not frozen, such as a string view (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary));
- printing a pointer inside another value (see [Not covered yet](#not-covered-yet));
- the forms in [Function values not covered yet](#function-values-not-covered-yet);
- the interface forms in [Interfaces not covered yet](#interfaces-not-covered-yet);
- the struct forms in [Struct forms not covered yet](#struct-forms-not-covered-yet), such as anonymous struct types and blank fields;
- untyped complex constant arguments of `go` and `defer`;
- goroutine launches of method expressions, interface method calls, and variadic calls, and launches with a nil function value (see [Unfrozen launch forms](./GOROUTINE.md#unfrozen-launch-forms));
- the forms in [Multiple results not covered yet](#multiple-results-not-covered-yet), such as `a, b = b, a`;
- `runtime.Goexit` (see [Panic and recover](#panic-and-recover));
- the forms in [Maps not covered yet](#maps-not-covered-yet);
- the forms in [Errors not covered yet](#errors-not-covered-yet) and [Formatted printing not covered yet](#formatted-printing-not-covered-yet);
- the forms in [The math/big package not covered yet](#the-mathbig-package-not-covered-yet);
- the forms in [The strings and bytes packages not covered yet](#the-strings-and-bytes-packages-not-covered-yet) and [The unicode package not covered yet](#the-unicode-package-not-covered-yet);
- the forms in [The io package not covered yet](#the-io-package-not-covered-yet), [The bufio package not covered yet](#the-bufio-package-not-covered-yet), and [The reflect package not covered yet](#the-reflect-package-not-covered-yet);
- the forms in [The strconv package not covered yet](#the-strconv-package-not-covered-yet), [The encoding/hex package not covered yet](#the-encodinghex-package-not-covered-yet), and [The encoding/binary package not covered yet](#the-encodingbinary-package-not-covered-yet);
- the forms in [Packages not covered yet](#packages-not-covered-yet), such as dot imports and test packages;
- the forms in [Evaluation order not covered yet](#evaluation-order-not-covered-yet), a loop post statement that needs temporaries, and in [Switch statements not covered yet](#switch-statements-not-covered-yet);
- `range` loops over functions, and the assignment form with a left operand that is not a variable (see [Range loops](#range-loops)), and the three-clause loop forms that [Loop clause variables](#loop-clause-variables) does not cover;
- zero-value construction for translated types other than basic types, strings, arrays, slices, maps, pointers, structs, interfaces, function types, and defined types;
- the forms in [Defined types not covered yet](#defined-types-not-covered-yet), such as defined pointer types;
- complex numbers and complex constants;
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
