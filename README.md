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
  go(f("hello"));
  go([]() -> func<> {
    fmt::Println("world");
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
  go([&]() -> func<> { co_await (ch << "ping"); });
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

A function containing Go `defer` declares one `eo_defer_scope` at function scope. Deferred callees, receivers, and arguments are saved in source order before `eo_defer(...)` registers a nullary callable. Translated normal exits call `eo_defer_run` before `return` or `co_return`, so deferred calls run in LIFO order while function locals are still alive.

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
  auto _eo_defer_arg_0_0 = std::string{"world"};
  eo_defer([=] { fmt::Println(_eo_defer_arg_0_0); });
  fmt::Println("hello");
  eo_defer_run;
  co_return;
}
```

A deferred closure can still be registered directly with `eo_defer([&] { ... });`. Defer statements inside loops and nested blocks register on the surrounding function's single defer stack. See `docs/TRANSLATION_RULES.md` for the canonical source-order and return translation shapes.

### Libraries

Some frequently used Go APIs are mirrored for easier source translation. Translation-facing APIs preserve the corresponding Go identifier spelling when it is a valid, non-reserved C++ identifier, so mechanically ported code does not need unrelated naming transformations. API resemblance does not imply complete behavioral equivalence; semantic compatibility is tested and documented separately as the project evolves.

See [Translation Principles](./docs/TRANSLATION.md) for the identifier-preservation policy and related translation rules.
