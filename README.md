# Eo

Eo provides syntactic sugars for C++ developers to write Go-like code.

Eo is not recommended for peformance-critical use case. This aims at migrating existing codebase from Go to C++ with minimum changes.

Eo is under active development and its APIs are not stable.

[Eo](https://en.wiktionary.org/wiki/eo#Latin) means "I **go**" in Latin and its pronunciation is ["Ay-Oh"](https://youtu.be/lkbP5OPQhdQ) like what Freddie Mercury shouted at Live Aid.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

## Requirements

Build dependencies will be installed by [Conan](https://github.com/conan-io/conan) package manager.

- C++20 (Clang 10+) (GCC support is temporarily disabled)
- Boost 1.78 (On MacOS, use [brew](https://brew.sh/) to install boost)
- [fmt](https://github.com/fmtlib/fmt)
- [scope-lite](https://github.com/martinmoene/scope-lite)

## Usage

For more examples, refer [here](https://github.com/conr2d/eo/tree/main/examples).

### Goroutine

Goroutine is emulated by C++20 stackless coroutine and `awaitable` of Boost.Asio.

`func<R = void>` is an alias type of `boost::asio::awaitable<R>`.

To generate awaitable function, `co_await` or `co_return` keyword should be in function body.

``` go
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

``` c++
// C++
func<> f(std::string s) {
  fmt::println(s);
  co_return; // if co_await keyword is used in function body, this can be omitted
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

Channel is emulated by `concurrent_channel` of Boost.Asio.

Receive operator `<-ch` and send operator `ch <-` are replaced by `*` and `<<` operators.

``` go
// Go
func main() {
  ch := make(chan string)
  go func() { ch <- "ping" }()
  msg := <-ch
  fmt.Println(msg);
}
```

``` c++
// C++
func<> eo_main() {
  auto ch = make_chan<std::string>();
  go([&]() -> func<> { co_await (ch << "ping"); });
  auto msg = co_await *ch;
  fmt::println(msg);
}
```

### Select statement

Select statement is emulated by `awaitable_operators` of Boost.Asio.

This has most different syntax from that of Go.

``` go
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

``` c++
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

Defer is emulated by scope-lite that implements experimental `std::scope_exit`.

For convenience, temporary variable for assigning `scope_exit` instance is generated by `eo_defer` macro.

``` go
// Go
func f() {
  defer fmt.Println("world")
  fmt.Println("hello")
}
```

``` c++
// C++
func<> f() {
  eo_defer([]() { fmt::println("world"); });
  fmt::println("hello");
}
```

### Libraries

Frequently used Go APIs are emulated, but their behaviors are not completely same.

For example, `fmt` of Go is emulated by `fmt` of C++, but its behavior follows original C++ API, not that of Go.
