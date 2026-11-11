# Eo

Eo is a C++ compatibility layer for porting existing Go code with minimal semantic and structural changes.

It is intentionally not an idiomatic C++ framework. The primary goal is to make mechanical Go-to-C++ translation practical when preserving the original control flow and concurrency structure matters more than redesigning the code around C++ conventions.

If there is enough time to rewrite a Go codebase properly in C++, that is usually the better choice. Eo exists for cases where a close, fast port is more useful than a clean rewrite.

Eo is under active development and its APIs are not stable.

[Eo](https://en.wiktionary.org/wiki/eo#Latin) means "I **go**" in Latin and its pronunciation is ["Ay-Oh"](https://youtu.be/lkbP5OPQhdQ), like what Freddie Mercury shouted at Live Aid.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

## Requirements

- C++23
- Boost.Asio with coroutine, experimental channel, and awaitable-operator support
- fmt
- CMake 3.25+

Dependencies are discovered through standard CMake `find_package` calls. Eo no longer downloads or configures a package manager during CMake configuration.

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
    auto _eo_defer_arg_0_0 = "world"s;
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
fmt::Println(len(s), at(s, 1), substr(s, 1, 3));
```

See `docs/TRANSLATION_RULES.md` for the literal escapes and the rest of the mapping.

### Libraries

Some frequently used Go APIs are mirrored for easier source translation. Translation-facing APIs preserve the corresponding Go identifier spelling when it is a valid, non-reserved C++ identifier, so mechanically ported code does not need unrelated naming transformations. API resemblance does not imply complete behavioral equivalence; semantic compatibility is tested and documented separately as the project evolves.

See [Translation Principles](./docs/TRANSLATION.md) for the identifier-preservation policy and related translation rules.
