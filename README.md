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
  fmt::println(s);
  co_return;
}

func<> eo_main() {
  go(f("hello"));
  go([]() -> func<> {
    fmt::println("world");
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
  fmt::println(msg);
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
  auto select = Select{*ch, CaseDefault()};
  for (;;) {
    switch (co_await select.index()) {
    case 0:
      auto msg = co_await select.process<0>();
      fmt::println(msg);
      break;
    default:
      co_return;
    }
  }
}
```

### Defer

`defer` is represented by the `eo_defer` macro, which uses Eo's lightweight internal RAII scope guard to run the deferred callable when the surrounding scope exits.

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
  eo_defer([]() { fmt::println("world"); });
  fmt::println("hello");
}
```

### Libraries

Some frequently used Go APIs are mirrored for easier source translation. API resemblance does not imply complete behavioral equivalence; semantic compatibility is tested and documented separately as the project evolves.
