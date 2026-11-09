# Goroutine Translation Design

This document defines the canonical Eo translation shape for Go `go` statements.

## Contract

A Go `go` statement evaluates the function value and call arguments in the launching goroutine. The call itself, including everything the callee does, happens in the new goroutine. Eo translations must preserve that boundary and the source evaluation order explicitly.

The translation therefore has two parts:

1. **Prepare.** Evaluate the function value, receiver, and arguments into adjacent temporaries in source order, in the launching goroutine.
2. **Launch.** Pass `go(...)` a nullary thunk that captures the prepared temporaries by value and performs only the call.

The runtime `go(...)` API invokes the thunk after launch. If the thunk returns an awaitable, `go(...)` runs it as the goroutine body. Otherwise the thunk's own execution is the goroutine body. The same launch shape is therefore used whether the translated callee is a coroutine or a plain function.

## Why the call goes inside the thunk

Launching a call expression directly, as in `go(f(args...))`, invokes the C++ function `f` before `go(...)` runs. That is only correct if every translated callee is a lazy coroutine whose body cannot run during invocation. A translated function that returns `eo::func<>` without being a coroutine, for example a wrapper that does some work and then returns another coroutine, would run that work in the launching goroutine.

The thunk moves only the call across the launch boundary. Every source expression has already been evaluated by the prepare step, so this does not change evaluation timing.

## Named function calls

```go
go f(x(), y())
```

```cpp
auto _eo_go_arg_0_0 = x();
auto _eo_go_arg_0_1 = y();
go([=] { return f(_eo_go_arg_0_0, _eo_go_arg_0_1); });
```

Temporary names use the source `go` statement ordinal followed by the argument ordinal. A call without arguments still uses the thunk: `go([=] { return f(); });`.

## Function-valued calls

If the callee is itself an expression, evaluate and save the function value before its arguments. The thunk is `mutable` so that a stateful function object can be called:

```go
go next()(x())
```

```cpp
auto _eo_go_fn_0 = next();
auto _eo_go_arg_0_0 = x();
go([=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0); });
```

## Method calls

For `go x.M(args...)`, evaluate and save the receiver binding before the arguments. Receiver binding follows the same Go value- vs pointer-receiver rules used by the defer translation contract.

For a value-receiver method, save the receiver value that Go would pass:

```cpp
auto _eo_go_receiver_0 = x;
auto _eo_go_arg_0_0 = y();
go([=]() mutable { return _eo_go_receiver_0.M(_eo_go_arg_0_0); });
```

For a pointer-receiver method whose receiver expression already evaluates to a pointer, save that pointer value directly:

```cpp
auto _eo_go_receiver_0 = p;
auto _eo_go_arg_0_0 = y();
go([=] { return _eo_go_receiver_0->M(_eo_go_arg_0_0); });
```

## Lifetime boundary

A launched goroutine can outlive the C++ scope that launched it. Go keeps any variable whose address escapes into a goroutine alive for as long as it is reachable. A raw C++ pointer or reference to a local of the launching scope does not.

This differs from defer. A deferred call runs at the function-level `eo_defer_run`, while the function's locals are still alive, so the defer contract may save `&x` for a pointer receiver. A goroutine has no such bound.

The following are therefore not frozen and must not be translated by saving a raw address of a launching-scope local:

- a pointer-receiver method called on an addressable local value (`go x.M()` where `M` has receiver `*T` and `x` is a local);
- an argument that takes the address of a local (`go f(&x)`);
- a function literal that captures a local by reference.

Until Eo defines a canonical mapping for escaping locals, a translator must leave these cases explicitly unsupported or provisional rather than emit `&x` or `[&]` and silently depend on C++ lifetime accidents. Pointers whose target storage is owned independently of the launching scope may be saved by value as shown above.

## Function literals

A function literal value is created in the launching goroutine and its body begins only after launch:

```go
go func() {
	work()
}()
```

```cpp
go([=]() -> func<> {
  work();
  co_return;
});
```

A direct C++ lambda may be used when its captures copy values whose sharing semantics already match the supported Go translation. Captures that would need Go-style shared variables or lifetime extension fall under the lifetime boundary above.

## Structural rule

Do not translate `go f(x())` as a wrapper whose body evaluates `x()` after launch:

```cpp
// Not canonical: x() runs in the launched goroutine.
go([&] { return f(x()); });
```

That changes observable evaluation timing and can reorder side effects, panics, or mutations relative to the launching goroutine.

The canonical thunk contains only the call of prepared temporaries. It never contains a source expression other than the call itself.
