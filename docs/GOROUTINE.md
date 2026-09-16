# Goroutine Translation Design

This document defines the canonical Eo translation shape for Go `go` statements.

## Contract

A Go `go` statement evaluates the function value and call arguments in the launching goroutine before the new goroutine begins executing the function body. Eo translations must preserve that evaluation timing and source order explicitly.

The runtime `go(...)` API only launches an already-prepared callable or awaitable. It must not be used to hide source expressions whose evaluation belongs to the launching goroutine.

## Named function calls

For a direct named function, evaluate argument expressions into adjacent temporaries in source order, then construct and launch the call:

```go
go f(x(), y())
```

```cpp
auto _eo_go_arg_0_0 = x();
auto _eo_go_arg_0_1 = y();
go(f(_eo_go_arg_0_0, _eo_go_arg_0_1));
```

The goroutine body must not begin before these evaluations complete.

## Function-valued calls

If the callee is itself an expression, evaluate and save the function value before its arguments:

```go
go next()(x())
```

```cpp
auto _eo_go_fn_0 = next();
auto _eo_go_arg_0_0 = x();
go(_eo_go_fn_0(_eo_go_arg_0_0));
```

Temporary names use the source `go` statement ordinal followed by the argument ordinal.

## Method calls

For `go x.M(args...)`, evaluate and save the receiver binding before the arguments. Receiver binding follows the same Go value- vs pointer-receiver rules used by the defer translation contract.

For a value-receiver method, save the receiver value that Go would pass:

```cpp
auto _eo_go_receiver_0 = x;
auto _eo_go_arg_0_0 = y();
go(_eo_go_receiver_0.M(_eo_go_arg_0_0));
```

For a pointer-receiver method invoked on an addressable value, preserve Go's implicit `&x` binding:

```cpp
auto* _eo_go_receiver_0 = &x;
auto _eo_go_arg_0_0 = y();
go(_eo_go_receiver_0->M(_eo_go_arg_0_0));
```

If the source receiver expression already evaluates to a pointer, evaluate and save that pointer value directly.

## Function literals

A function literal value is created in the launching goroutine and its body begins only after launch. A direct C++ lambda may be used when its capture semantics and lifetimes already match the supported Go translation. Cases requiring Go-style capture lifetime extension remain governed by the general translation lifetime rules and must not be approximated with dangling C++ references.

## Structural rule

Do not translate `go f(x())` as a wrapper whose body evaluates `x()` after launch:

```cpp
// Not canonical: x() runs in the launched task.
go([&]() -> func<> {
  co_await f(x());
});
```

That changes observable evaluation timing and can reorder side effects, panics, or mutations relative to the launching goroutine.

The canonical translation keeps evaluation adjacent to the source `go` statement and uses `go(...)` only for the launch boundary.
