# Goroutine Translation Design

This document defines the canonical Eo translation shape for Go `go` statements.

## Contract

A Go `go` statement evaluates the function value and call arguments in the launching goroutine. The call itself, including everything the callee does, happens in the new goroutine, and the launching goroutine does not wait for it. Eo translations must preserve that boundary and the source evaluation order explicitly.

The translation therefore has two parts:

1. **Prepare.** Evaluate the function value, receiver, and arguments into adjacent temporaries in source order, in the launching goroutine.
2. **Launch.** Pass `go(...)` a nullary `[=]() mutable` thunk that captures the prepared temporaries by value and performs only the call.

`go(...)` returns without running the thunk. The thunk is invoked on a later turn of the runtime executor, even when `go(...)` is called from a goroutine already running on that executor. If the thunk returns an awaitable, `go(...)` runs it as the goroutine body. Otherwise the thunk's own execution is the goroutine body. The same launch shape is therefore used whether the translated callee is a coroutine or a plain function.

## Why the call goes inside the thunk

Launching a call expression directly, as in `go(f(args...))`, invokes the C++ function `f` before `go(...)` runs. That is only correct if every translated callee is a lazy coroutine whose body cannot run during invocation. A translated function that returns `eo::func<>` without being a coroutine, for example a wrapper that does some work and then returns another coroutine, would run that work in the launching goroutine.

The thunk moves only the call across the launch boundary. Every source expression has already been evaluated by the prepare step, so this does not change evaluation timing.

The thunk is always `[=]() mutable`, so that the translator never has to decide whether the call needs a non-const function object or receiver.

## Named function calls

```go
go f(x(), y())
```

```cpp
auto _eo_go_arg_0_0 = x();
auto _eo_go_arg_0_1 = y();
go([=]() mutable { return f(_eo_go_arg_0_0, _eo_go_arg_0_1); });
```

Temporary names use the source `go` statement ordinal followed by the argument ordinal. A call without arguments still uses the thunk: `go([=]() mutable { return f(); });`.

## Function-valued calls

If the callee is itself an expression, evaluate and save the function value before its arguments:

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

For a pointer-receiver method whose receiver expression already evaluates to a pointer value, save that pointer value directly:

```cpp
auto _eo_go_receiver_0 = p;
auto _eo_go_arg_0_0 = y();
go([=]() mutable { return _eo_go_receiver_0->M(_eo_go_arg_0_0); });
```

## Lifetime boundary

A launched goroutine can outlive the C++ function that launched it. Go keeps any variable that a goroutine can reach alive and shared for as long as it is reachable. A raw C++ pointer or reference into the launching function's storage does not, and a by-value capture copies a variable that Go would share.

This differs from defer. A deferred call runs at the function-level `eo_defer_run`, while the function's locals are still alive, so the defer contract may save `&x` for a pointer receiver on any addressable value. A goroutine has no such bound.

Taking an address at a `go` statement (`&x`, `&s.f`, `&a[i]`, or the implicit `&x` of a pointer-receiver call on an addressable value) is frozen only when the storage is a package-level variable or a subobject of one. The following are not frozen:

- an address whose storage is, or is a subobject of, a local variable or parameter of the launching function, whether taken explicitly or implied by a pointer-receiver call;
- a function literal that captures a local variable or parameter by reference;
- a function literal that captures a local variable or parameter that either side writes after the `go` statement, because a by-value capture would not share it.

Until Eo defines a canonical mapping for escaping and shared locals, a translator must leave these cases explicitly unsupported or provisional rather than emit `&x`, `[&]`, or a by-value copy that silently changes semantics. Saving a pointer value that the source already holds, as in the `p` example above, is allowed.

## Function literals

A function literal value is created in the launching goroutine and its body begins only after launch:

```go
go func() {
	work()
}()
```

```cpp
go([=]() mutable -> func<> {
  work();
  co_return;
});
```

A direct C++ lambda may be used when it captures only values that neither side writes after the `go` statement. Other captures fall under the lifetime boundary above.

## Unfrozen launch forms

The following are not covered by this contract yet:

- method expressions such as `go T.M(x)` or `go (*T).M(&x)`;
- method calls on interface values, where Go evaluates the method value, and panics on a nil interface, in the launching goroutine;
- panics in the launched goroutine. A panic is currently discarded when the callee returns an awaitable and propagates out of the executor when it does not; Go terminates the program in both cases.

## Structural rule

Do not translate `go f(x())` as a wrapper whose body evaluates `x()` after launch:

```cpp
// Not canonical: x() runs in the launched goroutine.
go([&] { return f(x()); });
```

That changes observable evaluation timing and can reorder side effects, panics, or mutations relative to the launching goroutine.

The canonical thunk contains only the call of prepared temporaries. It never contains a source expression other than the call itself.

The examples in [`examples/`](../examples) predate this contract and may use other launch shapes.
