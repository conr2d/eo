# Goroutine Translation Design

This document defines the canonical Eo translation shape for Go `go` statements.

## Contract

A Go `go` statement evaluates the function value and call arguments in the launching goroutine. The call itself, including everything the callee does, happens in the new goroutine, and the launching goroutine does not wait for it. Eo translations must preserve that boundary and the source evaluation order explicitly.

The translation therefore has two parts:

1. **Prepare.** Evaluate the function value, receiver, and arguments into adjacent temporaries in source order, in the launching goroutine.
2. **Launch.** Pass `go(...)` a nullary `[=]() mutable` thunk that captures the prepared temporaries by value and performs only the call.

`go(...)` returns without running the thunk. The thunk is invoked on a later turn of the runtime executor, even when `go(...)` is called from a goroutine already running on that executor. If the thunk returns an awaitable, `go(...)` runs it as the goroutine body. Otherwise the thunk's own execution is the goroutine body. The same launch shape is therefore used whether the translated callee is a coroutine or a plain function.

Go-to-C++ type mapping is outside this contract. The C++ types in the examples are illustrative.

## Why the call goes inside the thunk

Launching a call expression directly, as in `go(f(args...))`, invokes the C++ function `f` before `go(...)` runs. That is only correct if every translated callee is a lazy coroutine whose body cannot run during invocation. A translated function that returns `eo::func<>` without being a coroutine, for example a wrapper that does some work and then returns another coroutine, would run that work in the launching goroutine.

The thunk moves only the call across the launch boundary. Every source expression has already been evaluated by the prepare step, so this does not change evaluation timing.

The thunk is always `[=]() mutable`, so that the translator never has to decide whether the call needs a non-const function object or receiver.

## Named function calls

```go
go f(x(), y()) // func f(a int64, b string)
```

```cpp
int64_t _eo_go_arg_0_0 = x();
std::string _eo_go_arg_0_1 = y();
go([=]() mutable { return f(_eo_go_arg_0_0, _eo_go_arg_0_1); });
```

A call without arguments still uses the thunk: `go([=]() mutable { return f(); });`.

### Argument temporary types

Declare each argument temporary with the value type of the translated parameter it is passed to, not `auto`. The value type is the translated parameter type with reference and `const`/`volatile` qualifiers removed, so the temporary always holds its own copy: a parameter translated as `const std::string&` gives a `std::string` temporary. Go converts each argument to its parameter type when the `go` statement evaluates it, so the conversion belongs in the launching goroutine. With `auto`, the temporary would keep the argument's own C++ type: `nil` would become `std::nullptr_t`, and a value passed to an interface parameter would not be converted until the call.

An untyped constant argument takes the parameter type in Go. Write the constant expression so that it is evaluated in that type, by writing its first untyped operand as a value of that type:

```go
go f(1 << 40) // func f(n int64)
```

```cpp
int64_t _eo_go_arg_0_0 = int64_t{1} << 40;
go([=]() mutable { return f(_eo_go_arg_0_0); });
```

If the parameter has an interface type, the constant takes its Go default type instead (`bool`, `rune`, `int`, `float64`, `complex128`, or `string`), and the expression is written in that type before the conversion to the interface. A constant expression whose exact Go value cannot be computed in that C++ type, for example because an intermediate value overflows it, is not frozen.

Function-value and receiver temporaries are declared with `auto`, because Go does not convert them.

### Temporary names

Temporaries are named `_eo_go_fn_N`, `_eo_go_receiver_N`, and `_eo_go_arg_N_I`, where `N` is the ordinal of the `go` statement and `I` is the ordinal of the argument, both starting at 0.

`go` statements are counted in source order across one outermost Go function: a function or method declaration, or a function literal that is not inside another function, such as one in a package-level variable initializer. `go` statements inside nested function literals belong to the same count, so a nested literal never declares a temporary that shadows one of an enclosing function.

## Function-valued calls

If the callee is itself an expression, evaluate and save the function value before its arguments:

```go
go next()(x()) // next returns func(int64)
```

```cpp
auto _eo_go_fn_0 = next();
int64_t _eo_go_arg_0_0 = x();
go([=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0); });
```

## Method calls

For `go x.M(args...)`, evaluate and save the receiver binding before the arguments. Receiver binding follows the same Go value- vs pointer-receiver rules used by the defer translation contract.

For a value-receiver method, save the receiver value that Go would pass. When Go selects a value-receiver method through a pointer `p`, that value is `*p`, so the dereference happens in the launching goroutine.

```cpp
auto _eo_go_receiver_0 = x;
int64_t _eo_go_arg_0_0 = y();
go([=]() mutable { return _eo_go_receiver_0.M(_eo_go_arg_0_0); });
```

For a pointer-receiver method whose receiver expression already evaluates to a pointer value, save that pointer value directly:

```cpp
auto _eo_go_receiver_0 = p;
int64_t _eo_go_arg_0_0 = y();
go([=]() mutable { return _eo_go_receiver_0->M(_eo_go_arg_0_0); });
```

This copies the pointer, not its target. It is sound only when the target outlives the goroutine; see the lifetime boundary below.

## Lifetime boundary

A launched goroutine can outlive the C++ function that launched it. Go keeps any variable that a goroutine can reach alive and shared for as long as it is reachable. A raw C++ pointer or reference into the launching function's storage does not, and a by-value capture copies a variable that Go would share.

This differs from defer. A deferred call runs at the function-level `eo_defer_run`, while the function's locals are still alive, so the defer contract may save `&x` for a pointer receiver on any addressable value. A goroutine has no such bound.

Taking an address at a `go` statement (`&x`, `&s.f`, `&a[i]`, or the implicit `&x` of a pointer-receiver call on an addressable value) is frozen only when the storage is a package-level variable or a subobject of one. That case saves the address like the defer pointer-receiver shape:

```cpp
auto* _eo_go_receiver_0 = &g; // g is a package-level variable; M has receiver *T
go([=]() mutable { return _eo_go_receiver_0->M(); });
```

The following are not frozen:

- an address whose storage is, or is a subobject of, a local variable or parameter of the launching function, whether taken explicitly or implied by a pointer-receiver call;
- a function literal that captures a local variable or parameter by reference;
- a function literal that captures by value a local variable or parameter that is written after the `go` statement, as defined in [By-value captures](#by-value-captures), because a by-value capture would not share it.

Until Eo defines a canonical mapping for escaping and shared locals, a translator must leave these cases explicitly unsupported or provisional rather than emit `&x`, `[&]`, or a by-value copy that silently changes semantics.

Saving a pointer value that the source already holds, as in the `p` example above, takes no new address at the `go` statement, but it does not keep the target alive either. If the target is a local variable or parameter, of the launching function or of a caller, the goroutine can outlive it exactly as in the first case above. This applies to a pointer passed as a receiver, as an argument, or through a by-value capture.

Such a pointer is frozen only when its source can be checked in the launching function: the expression is `new(T)`, `&T{...}`, or the address of package-level storage, or it is a local variable of the launching function whose declaration and every assignment are such expressions and whose address is not taken. A pointer from any other source, such as a parameter, a field, a package-level pointer variable, or a function result, cannot be checked locally. The translator uses the same shape and marks the launch as provisional, because its soundness depends on where the pointer came from.

## Function literals

A function literal is a function value. Its closure is created in the launching goroutine, when the `go` statement evaluates the function value. The literal's body is the callee body, so it begins only after launch.

A literal with no parameters, called without arguments, is passed to `go(...)` directly. The lambda is both the function value and the launch thunk:

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

The rule that the thunk contains only the call applies to the call, which here has no arguments to prepare. The lambda body is the translated body of the literal, not a source expression moved across the launch.

Any other literal is a function-valued call. Save the literal in `_eo_go_fn_N`, prepare its arguments, and launch the usual thunk:

```go
go func(a int64) {
	work(a)
}(x())
```

```cpp
auto _eo_go_fn_0 = [=](int64_t a) mutable -> func<> {
  work(a);
  co_return;
};
int64_t _eo_go_arg_0_0 = x();
go([=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0); });
```

Package-level variables are not captured. The lambda refers to them directly, so they are shared as in Go.

### By-value captures

`[=]` copies each captured local variable or parameter of the launching function when the lambda is created, while Go shares the variable. The copy is equivalent only if nothing writes the variable after that point. A literal launched by `go` may capture a local variable or parameter `v` by value only if there is no write to `v` as defined below. Otherwise the launch falls under the [lifetime boundary](#lifetime-boundary) and is unsupported.

A part of `v` is `v` followed by field selectors and array index expressions that stay inside `v`'s own storage, such as `v.f`, `v[i]` where `v` is an array, or `v.f[i].g`. A selector or index that goes through a pointer, slice, or map, including an implicit pointer dereference, reaches storage that `v` refers to rather than `v` itself. This assumes the translated pointer, slice, and map types share their target when copied, as Go's do.

Each of the following is a write to `v`:

- an assignment or compound assignment whose target is `v` or a part of `v`, including one target of a multiple assignment, a receive assignment such as `v, ok = <-ch` or a `select` case `case v = <-ch:`, and a short variable declaration that redeclares `v`;
- `++` or `--` applied to `v` or a part of `v`;
- `v` or a part of `v` used as an iteration variable of a `range` clause with `=`;
- a `return` statement with operands, which writes every named result parameter of the function it returns from;
- taking the address of `v` or a part of `v`, explicitly or implicitly: `&v`, `&v.f`, `&v[i]`, a call or method value of a pointer-receiver method on `v` or a part of `v` (`v.Lock()`, `v.f.Add(1)`, `v.M`), or any slice expression (`v[:]`, `v[a:b]`, `v[a:b:c]`) on `v` or a part of `v` that is an array.

Reading `v`, passing it by value, and calling a value-receiver method on it are not writes.

A write counts if it is:

- anywhere in the body of the launched literal;
- anywhere in another function literal that captures `v`, because it can run at any time;
- in the launching function, anywhere in `v`'s scope that can run after the `go` statement: any statement after it in source order, and every part of a loop that encloses the `go` statement and is itself inside `v`'s scope, including its body, condition, post statement, and `range` clause. If a `goto` can jump backward over the `go` statement inside `v`'s scope, every statement in `v`'s scope counts.

Taking the address of `v` counts wherever it appears in `v`'s scope, including before the `go` statement, because writes through the resulting pointer cannot be checked.

### Loop variables

Eo follows Go 1.22 and later loop-variable semantics. Each iteration of a `for` loop whose clause declares variables with `:=`, in the three-clause or `range` form, has its own copy of those variables. Code from a module whose Go language version is earlier than 1.22 is not covered when a launched literal captures a loop variable.

For the write check, each iteration's loop variable is a separate variable whose scope is that iteration. The loop's own updates, the post statement of a three-clause loop and the assignment of the next `range` values, act on the next iteration's variable and are not writes to the variable captured in the current iteration. A write to the loop variable inside the loop body is a write to the current iteration's variable, and it counts if it can run after the `go` statement.

The exception for the loop's own updates covers only the variables that the loop's own clause declares with `:=`. A post statement or `range ... =` clause of an enclosing loop that writes any other variable, including the loop variable of an outer loop, counts as above:

```go
x := 0
for ; x < n; x++ { // x++ writes the x that every launched literal shares
	go func() {
		use(x)
	}()
}
```

The canonical C++ shape keeps the Go clauses in a C++ `for` and declares the loop variables in its init clause:

```go
for i := 0; i < n; i++ {
	go func() {
		use(i)
	}()
}
```

```cpp
for (int i = 0; i < n; i++) {
  go([=]() mutable -> func<> {
    use(i);
    co_return;
  });
}
```

While the body runs, the C++ `i` stands for the current iteration's Go variable. The by-value capture copies it at the `go` statement, and since nothing writes that iteration's variable afterward, the copy stays equal to it. The C++ `i++` runs after the copy is made and corresponds to the Go post statement on the next iteration's variable.

## Unfrozen launch forms

The following are not covered by this contract yet:

- method expressions such as `go T.M(x)` or `go (*T).M(&x)`;
- method calls on interface values, where Go evaluates the method value, and panics on a nil interface, in the launching goroutine;
- calls to a variadic function, such as `go f(a, b, c)` or `go f(xs...)` with a `...T` parameter, because Eo has no frozen mapping for the variadic slice that Go builds or passes in the launching goroutine;
- multi-value calls such as `go f(g())`, where one call supplies several arguments;
- a nil function value. For `go f()` with a nil `f`, Go stops the program in the launching goroutine with the fatal error `go of nil func value` before the statement completes. With this mapping the launch completes, and the failure, if any, happens later in the launched goroutine: undefined behavior for a raw function pointer, or an exception for a `std::function`;
- a nil pointer `p` in `go p.M()` where `M` has a value receiver. Go panics in the launching goroutine. The prepare step dereferences `p` at the same point, but dereferencing a null C++ pointer is undefined behavior rather than a panic;
- panics in the launched goroutine. A panic is currently discarded when the callee returns an awaitable and propagates out of the executor when it does not; Go terminates the program in both cases.

The nil cases are instances of the panic-producing operations listed under [Unfrozen constructs](./TRANSLATION_RULES.md#unfrozen-constructs). The shapes above remain canonical when the function value and receiver pointer are not nil.

## Structural rule

Do not translate `go f(x())` as a wrapper whose body evaluates `x()` after launch:

```cpp
// Not canonical: x() runs in the launched goroutine.
go([&] { return f(x()); });
```

That changes observable evaluation timing and can reorder side effects, panics, or mutations relative to the launching goroutine.

The canonical thunk contains only the call of prepared temporaries. It never contains a source expression other than the call itself. A function literal passed directly to `go(...)` is not an exception, because its body is the callee body.

The examples in [`examples/`](../examples) predate this contract and may use other launch shapes.
