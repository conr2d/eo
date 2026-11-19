# Eo

Eo is a C++ compatibility layer for porting existing Go code with minimal semantic and structural changes.

It is intentionally not an idiomatic C++ framework. The primary goal is to make mechanical Go-to-C++ translation practical when preserving the original control flow and concurrency structure matters more than redesigning the code around C++ conventions.

If there is enough time to rewrite a Go codebase properly in C++, that is usually the better choice. Eo exists for cases where a close, fast port is more useful than a clean rewrite.

Eo is under active development and its APIs are not stable.

[Eo](https://en.wiktionary.org/wiki/eo#Latin) means "I **go**" in Latin and its pronunciation is ["Ay-Oh"](https://youtu.be/lkbP5OPQhdQ), like what Freddie Mercury shouted at Live Aid.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

## Requirements

- C++23 on a 64-bit platform, with GCC or Clang, which accept `-fwrapv`
- Boost.Asio with coroutine, experimental channel, and awaitable-operator support
- fmt
- CMake 3.25+

The `eo` target adds `-fwrapv` to every target that links it, so signed overflow wraps as in Go. Dependencies are discovered through standard CMake `find_package` calls. Eo no longer downloads or configures a package manager during CMake configuration.

To build with sanitizers, pass the `-fsanitize=` list as `EO_SANITIZE`, for example `-DEO_SANITIZE=address,undefined` or `-DEO_SANITIZE=thread`. CI runs the test suite both ways, once with the default worker count and once with `EOMAXPROCS=1`.

## Usage

For more examples, refer [here](https://github.com/conr2d/eo/tree/main/examples).

### Goroutine

Goroutines are emulated with C++ stackless coroutines and Boost.Asio `awaitable`.

`func<R = void>` is an alias for `boost::asio::awaitable<R>`.

```go
// Go
func f(s string) {
  fmt.Println(s)
}

func main() {
  go f("hello")
  go func() {
    fmt.Println("world")
  }()
}
```

```cpp
// C++
func<> f(std::string s) {
  fmt::Println(s);
  co_return;
}

func<> eo_main() {
  go(f("hello"s));
  go([]() -> func<> {
    fmt::Println("world"s);
    co_return;
  });
  co_return;
}
```

### Channel

Channels are currently implemented on top of Boost.Asio experimental concurrent channels.

Go receive `<-ch` and send `ch <- value` are represented by `*ch` and `ch << value`.

```go
// Go
func main() {
  ch := make(chan string)
  go func() { ch <- "ping" }()
  msg := <-ch
  fmt.Println(msg)
}
```

```cpp
// C++
func<> eo_main() {
  auto ch = make_chan<std::string>();
  go([&]() -> func<> { co_await (ch << "ping"s); });
  auto msg = co_await *ch;
  fmt::Println(msg);
}
```

### Select statement

Select is currently implemented with Boost.Asio awaitable composition.

```go
// Go
func f() {
  for {
    select {
    case msg := <-ch:
      fmt.Println(msg)
    default:
      return
    }
  }
}
```

```cpp
// C++
func<> f() {
  for (;;) {
    switch (auto select = Select{*ch}; select.try_index()) {
    case 0: {
      auto msg = select.recv<0>();
      fmt::Println(msg);
      break;
    }
    default: {
      co_return;
    }
    }
  }
}
```

### Defer

A function containing Go `defer` declares one `eo_defer_scope` at function scope. Deferred callees, receivers, and arguments are saved in source order before `eo_defer(...)` registers a nullary callable. Translated normal exits call `eo_defer_run` before `return` or `co_return`, so deferred calls run in LIFO order while function locals are still alive. From the first statement that contains a defer, the body is wrapped in `try`, and the catch handler runs the deferred calls on a panic with `eo_defer_panic`, where `recover()` can stop it.

```go
// Go
func f() {
  defer fmt.Println("world")
  fmt.Println("hello")
}
```

```cpp
// C++
func<> f() {
  eo_defer_scope;
  try {
    auto _eo_defer_arg_0_0 = std::string{"world"s};
    eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
    fmt::Println("hello"s);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
```

Deferred method receivers follow Go's receiver binding: value receivers save a value, while pointer receivers on addressable values save the corresponding address. A deferred closure uses `eo_defer([&] { ... });` when every variable it captures lives until the function-level drain, and copies the variables declared inside the `try` block otherwise. See `docs/TRANSLATION_RULES.md` for the canonical source-order, receiver, and return translation shapes.

### Basic types

Go `int` is `int64_t`, and the other basic types map to their fixed-width C++ types. Division, remainder, and shifts go through functions that panic and wrap as Go does, and an untyped constant is written from its folded value where C++ would compute it differently.

```go
// Go
n := 10
var b int8 = 100
fmt.Println(n/d, n<<k, b+b, 1<<40)
```

```cpp
// C++
auto n = int64_t{10};
int8_t b = 100;
fmt::Println(eo::div(n, d), eo::shl(n, k), int8_t(b + b), int64_t{1099511627776});
```

See `docs/TRANSLATION_RULES.md` for the full mapping, including printing floats as Go does.

### Pointers

Go `*T` is `ptr<T>`. A variable that the Go compiler moves to heap (`go build -gcflags='all=-l -m=2'` reports `moved to heap: x`) is boxed with `New<T>(...)`, so goroutines, closures, and returned pointers share it and keep it alive. Copies share the target, and dereferencing nil panics with Go's runtime error.

```go
// Go
func newCounter() *int {
  n := 0
  return &n
}
```

```cpp
// C++
auto newCounter() -> ptr<int64_t> {
  auto n = New<int64_t>(0);
  return n;
}
```

A pointer is reference-counted, so a reference cycle is never freed. See `docs/TRANSLATION_RULES.md` for the full mapping, including `ref(x)` for addresses that do not escape and the static method form.

### Strings

Go `string` is `std::string`, and a string literal is `"..."s`. Indexing, slicing, and `len` go through functions that keep Go's byte semantics and panic with Go's runtime error out of range.

```go
// Go
s := "héllo"
fmt.Println(len(s), s[1], s[1:3])
```

```cpp
// C++
auto s = "héllo"s;
fmt::Println(eo::len(s), eo::at(s, 1), eo::substr(s, 1, 3));
```

See `docs/TRANSLATION_RULES.md` for the literal escapes and the rest of the mapping.

### Slices

Go `[]T` is `slice<T>`, a view of a shared backing array, so copies and subslices share elements as in Go. `append` writes in place within capacity and copies past it, and indexing and slicing panic with Go's runtime error out of range.

```go
// Go
a := []int{1, 2, 3}
t := a[1:2]
t = append(t, 7) // writes a[2]
fmt.Println(a, len(t), cap(t))
```

```cpp
// C++
auto a = slice<int64_t>::of({1, 2, 3});
auto t = eo::subslice(a, 1, 2);
t = eo::append(t, {7}); // writes a[2]
fmt::Println(a, eo::len(t), eo::cap(t));
```

`slice<T>{}` is the nil slice, and a composite literal is always `slice<T>::of({...})`, so `[]T{}` stays non-nil. See `docs/TRANSLATION_RULES.md` for the full mapping.

### Maps

Go `map[K]V` is `map<K, V>`, a handle to a shared hash table, so copies and parameters share entries as in Go. Reading a missing key returns the zero value without inserting it, and assigning to an entry of a nil map panics with Go's message.

```go
// Go
counts := map[string]int{"a": 1}
counts["b"]++
v, ok := counts["c"]
delete(counts, "a")
fmt.Println(counts, len(counts), v, ok)
```

```cpp
// C++
auto counts = map<std::string, int64_t>::of({{"a"s, 1}});
eo::map_entry(counts, "b"s)++;
auto [v, ok] = eo::map_get_ok(counts, "c"s);
eo::delete_(counts, "a"s);
fmt::Println(counts, eo::len(counts), v, ok);
```

`fmt` prints a map in sorted key order, as Go does, and a `range` loop over a map does not produce an entry deleted before it is reached. See `docs/TRANSLATION_RULES.md` for the full mapping and the `maps` package.

### Range loops

`for range` over an integer, a slice, a string, or a map is a range-based `for` over `eo::range(x)`, which evaluates `x` once and decodes a string as UTF-8, as Go does. Each iteration declares new variables.

```go
// Go
for i, v := range s {
	fmt.Println(i, v)
}
for i, r := range "h\u00e9" {
	fmt.Println(i, r)
}
```

```cpp
// C++
for (auto [i, v] : eo::range(s)) {
  fmt::Println(i, v);
}
for (auto [i, r] : eo::range("h\u00e9"s)) {
  fmt::Println(i, r);
}
```

### Structs

A Go struct is a C++ aggregate whose fields start as their zero values, with a defaulted `==`. Keyed literals become designated initializers, embedded fields become members named after their types, and `fmt::Println` prints a struct as Go does.

```go
// Go
type Point struct {
	X, Y int
}

p := Point{Y: 2}
q := &Point{1, 2}
fmt.Println(p, *q == p, q)
```

```cpp
// C++
struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

auto p = Point{.Y = 2};
auto q = New<Point>(Point{1, 2});
fmt::Println(p, *q == p, q); // {0 2} false &{1 2}
```

See `docs/TRANSLATION_RULES.md` for declaration order, embedding, and names that members hide.

### Interfaces

A Go interface is a class derived from `eo::interface<I>`. A translated type lists its methods in `_eo_methods`, and a conversion finds the interface's methods there by name, so satisfaction stays implicit, as in Go. Nil and typed nil, equality, type assertions, and type switches behave as in Go, and `fmt::Println` calls `Error` and `String` methods.

```go
// Go
type Shape interface {
	Area() int
}

func (r Rect) Area() int { return r.W * r.H }

var s Shape = Rect{W: 2, H: 3}
r, ok := s.(Rect)
fmt.Println(s.Area(), r.W, ok)
```

```cpp
// C++
struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Shape";

  auto Area() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">>;
};

struct Rect {
  static constexpr std::string_view _eo_name = "main.Rect";

  int64_t W{}, H{};

  auto operator==(const Rect&) const -> bool = default;

  static auto Area(Rect r) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Rect::Area, "Area">>;
};

auto Shape::Area() const -> int64_t {
  return _eo_call<&Shape::Area>();
}

auto Rect::Area(Rect r) -> int64_t {
  return r.W * r.H;
}

Shape s = Rect{.W = 2, .H = 3};
auto [r, ok] = eo::type_assert_ok<Rect>(s);
fmt::Println(s.Area(), r.W, ok); // 6 2 true
```

See `docs/TRANSLATION_RULES.md` for method tables, promoted methods, and type switches.

### Multiple results

A Go function with several results returns a `std::tuple`. The results are declared with a structured binding, assigned with `std::tie`, and passed on to another call with `std::apply`.

```go
// Go
func divmod(a, b int) (int, int) {
	return a / b, a % b
}

q, r := divmod(17, 5)
q, _ = divmod(9, 4)
fmt.Println(divmod(7, 2))
```

```cpp
// C++
auto divmod(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  return {eo::div(a, b), eo::rem(a, b)};
}

auto [q, r] = divmod(17, 5);
q = std::get<0>(divmod(9, 4));
std::apply(fmt::Println, divmod(7, 2)); // 3 1
```

### Errors

Go's `error` is the interface `error`, and the `errors` package and `fmt.Errorf` keep Go's wrapping: `errors.Is` and `errors.As` see through `%w`, and two `errors.New` values with the same text are not equal. `fmt::Sprintf` and `fmt::Printf` take Go's format verbs. A recovered panic value is an `any`, and Go's run-time errors are `runtime.Error` values.

```go
// Go
func load(key string) (int, error) {
	if key == "" {
		return 0, fmt.Errorf("load %q: %w", key, errors.New("empty key"))
	}
	return len(key), nil
}

_, err := load("")
fmt.Println(err, errors.Unwrap(err))
```

```cpp
// C++
auto load(std::string key) -> std::tuple<int64_t, error> {
  if (key == ""s) {
    return {0, fmt::Errorf("load %q: %w"s, key, errors::New("empty key"s))};
  }
  return {eo::len(key), nullptr};
}

auto err = std::get<1>(load(""s));
fmt::Println(err, errors::Unwrap(err)); // load "": empty key empty key
```

See `docs/TRANSLATION_RULES.md` for the verbs that are covered and the run-time error types.

### Libraries

Some frequently used Go APIs are mirrored for easier source translation. Translation-facing APIs preserve the corresponding Go identifier spelling when it is a valid, non-reserved C++ identifier, so mechanically ported code does not need unrelated naming transformations. API resemblance does not imply complete behavioral equivalence; semantic compatibility is tested and documented separately as the project evolves.

See [Translation Principles](./docs/TRANSLATION.md) for the identifier-preservation policy and related translation rules.
