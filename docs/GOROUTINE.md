# Goroutine Translation Design

This document defines the canonical Eo translation shape for Go `go` statements.

## Contract

A Go `go` statement evaluates the function value and call arguments in the launching goroutine. The call itself, including everything the callee does, happens in the new goroutine, and the launching goroutine does not wait for it. Eo translations must preserve that boundary and the source evaluation order explicitly.

The translation therefore has two parts:

1. **Prepare.** Evaluate the function value, receiver, and arguments into adjacent temporaries in source order, in the launching goroutine.
2. **Launch.** Pass `go(...)` a nullary `[=]() mutable` thunk that captures the prepared temporaries by value and performs only the call.

`go(...)` returns without running the thunk. The thunk is invoked on a later turn of the runtime executor, even when `go(...)` is called from a goroutine already running on that executor. If the thunk returns an awaitable, `go(...)` runs it as the goroutine body. Otherwise the thunk's own execution is the goroutine body. The same launch shape is therefore used whether the translated callee is a coroutine or a plain function.

Go-to-C++ type mapping is outside this contract. The C++ types in the examples are illustrative.

Two outcomes are used for cases this contract does not freeze. An **unsupported** case has no translation: the translator emits none and leaves the construct failing visibly, as described under [Unsupported constructs](./TRANSLATION_RULES.md#unsupported-constructs). A **provisional** case uses the shape given here, but the translator marks the launch provisional, as described under [Unfrozen constructs](./TRANSLATION_RULES.md#unfrozen-constructs).

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

Declare each argument temporary with the value type of the translated parameter it is passed to, not `auto`. The value type is the translated parameter type with reference and `const`/`volatile` qualifiers removed, so the temporary holds its own copy whenever that type owns its value: a parameter translated as `const T&` gives a `T` temporary. Go strings are `std::string` parameters (see [Strings](./TRANSLATION_RULES.md#strings)). If the translated parameter type is a non-owning view, such as `std::string_view` or `std::span`, the temporary would refer to storage that may not outlive the launch, and the launch is provisional. Go converts each argument to its parameter type when the `go` statement evaluates it, so the conversion belongs in the launching goroutine. With `auto`, the temporary would keep the argument's own C++ type: `nil` would become `std::nullptr_t`, and a value passed to an interface parameter would not be converted until the call.

An untyped boolean, integer, floating-point, or string constant argument, including a named untyped constant, is written as its folded value, as defined in [Untyped constant arguments](./TRANSLATION_RULES.md#untyped-constant-arguments), which also covers complex constants:

```go
go f(1 << 40) // func f(n int64)
```

```cpp
int64_t _eo_go_arg_0_0 = int64_t{1099511627776};
go([=]() mutable { return f(_eo_go_arg_0_0); });
```

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

For a value-receiver method, save the receiver value that Go would pass. When Go selects a value-receiver method through a pointer `p`, that value is `*p`, so the dereference happens in the launching goroutine, and a nil `p` panics there, as in Go.

```cpp
auto _eo_go_receiver_0 = x;
int64_t _eo_go_arg_0_0 = y();
go([=]() mutable { return T::M(_eo_go_receiver_0, _eo_go_arg_0_0); });
```

Both receiver kinds call the method in its [static form](./TRANSLATION_RULES.md#methods), `T::M(receiver, args...)`. For a pointer-receiver method, save the receiver as a pointer. If the receiver expression is already a pointer `p`, save `p`. If it is an addressable value `x`, save `&x`, translated as described in [Pointers and escaping variables](./TRANSLATION_RULES.md#pointers-and-escaping-variables):

```cpp
auto _eo_go_receiver_0 = p;
int64_t _eo_go_arg_0_0 = y();
go([=]() mutable { return T::M(_eo_go_receiver_0, _eo_go_arg_0_0); });
```

A method of an Eo library type, such as `sync::WaitGroup`, is a member function, so the thunk calls it through the saved pointer: `return _eo_go_receiver_0->M(_eo_go_arg_0_0);`.

## Lifetime boundary

A launched goroutine can outlive the C++ function that launched it. Go keeps any variable that a goroutine can reach alive and shared for as long as it is reachable. A raw C++ pointer or reference into the launching function's storage does not, and a by-value capture copies a variable that Go would share.

[Pointers and escaping variables](./TRANSLATION_RULES.md#pointers-and-escaping-variables) closes this gap on the side that declares the variable, not at the `go` statement. A variable that a goroutine can reach, through a pointer, a capture, or a pointer-receiver call, is reported moved to heap by the Go compiler and is boxed. Its address is then an owning `ptr<T>`, and a capture copies the box, which shares the variable. A `ref` pointer refers only to storage that Go proved does not outlive its function, so no goroutine can hold one. The launch itself takes no special form:

```go
func (s *T) Start() {
	go s.loop()
}
```

```cpp
void T::Start(ptr<T> s) {
  auto _eo_go_receiver_0 = s;
  go([=]() mutable { return T::loop(_eo_go_receiver_0); });
}
```

The saved `s` keeps the target alive whatever the caller passed. A caller that writes `var t T; t.Start()` boxes `t`, because `Start` keeps its receiver.

Every address and every held pointer at a `go` statement is therefore frozen, whatever its source: an argument, a receiver, a capture, or a parameter of the launching function. The boundary still applies to values that refer to storage through a type whose mapping is not frozen, such as a slice of a local array, a string view, or an interface. Those launches are provisional until the mapping of that type defines how it keeps its storage alive.

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

### Captures

A launched literal captures as described in [Function literals](./TRANSLATION_RULES.md#function-literals). `[=]` copies each captured variable of the launching function, and of any enclosing function, when the lambda is created. Go captures by value only a variable that is never written after it is captured, so the copy equals the variable. Every variable that a launched literal captures by reference is moved to heap and boxed, so the copy is a pointer that shares the variable:

```go
x := 0
done := make(chan struct{})
res := make(chan int)
start := func() {
	go func() { <-done; res <- x }() // x is declared in the enclosing function
}
start()
x = 1
close(done)
fmt.Println(<-res) // prints 1
```

```cpp
auto x = New<int64_t>(0);
// ...
auto start = [=]() mutable {
  go([=]() mutable -> func<> {
    co_await *done;
    co_await (res << *x);
  });
};
start();
*x = 1;
```

Both lambdas copy the box `x`, so the goroutine reads the 1 that `main` writes later. The translator does not check where a variable is written. The escape facts already decide it.

### Loop variables

Eo follows Go 1.22 and later loop-variable semantics. Each iteration of a `for` loop whose clause declares variables with `:=`, in the three-clause or `range` form, has its own copy of those variables. Code from a module whose Go language version is earlier than 1.22 is not covered when a launched literal captures a loop variable.

Each iteration's loop variable is a separate variable. The loop's own updates, the post statement of a three-clause loop and the assignment of the next `range` values, act on the next iteration's variable. Go captures the current iteration's variable by value only if nothing writes it or takes its address after the capture and it is at most 128 bytes, so a launched literal usually copies it. A loop variable that a launched literal captures by reference is boxed, and a boxed three-clause loop variable gets a new box for every iteration, so each goroutine keeps its own iteration's variable (see [Loop clause variables](./TRANSLATION_RULES.md#loop-clause-variables)). A boxed `range` variable is not covered yet (see [#84](https://github.com/conr2d/eo/issues/84)).

This covers only the variables that the loop's own clause declares with `:=`. A variable declared before the loop is one variable for every iteration. A launched literal that captures it while the loop's post statement writes it captures it by reference, so it is boxed, and every goroutine shares it, as in Go:

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

While the body runs, the C++ `i` stands for the current iteration's Go variable. Go captures it by value, and the `[=]` capture copies it at the `go` statement. Nothing writes that iteration's variable afterward, so the copy stays equal to it. The C++ `i++` runs after the copy is made and corresponds to the Go post statement on the next iteration's variable.

## Unfrozen launch forms

The following are not covered by this contract yet:

- method expressions such as `go T.M(x)` or `go (*T).M(&x)`;
- method calls on interface values, where Go evaluates the method value, and panics on a nil interface, in the launching goroutine;
- calls to a variadic function, such as `go f(a, b, c)` or `go f(xs...)` with a `...T` parameter, because Eo has no frozen mapping for the variadic slice that Go builds or passes in the launching goroutine;
- multi-value calls such as `go f(g())`, where one call supplies several arguments;
- untyped complex constant arguments (see [Untyped constant arguments](./TRANSLATION_RULES.md#untyped-constant-arguments));
- a nil function value. For `go f()` with a nil `f`, Go stops the program in the launching goroutine with the fatal error `go of nil func value` before the statement completes. With this mapping the launch completes, and the failure, if any, happens later in the launched goroutine: undefined behavior for a raw function pointer, or an exception for a `std::function`.

The nil function value is an instance of the panic-producing operations listed under [Unfrozen constructs](./TRANSLATION_RULES.md#unfrozen-constructs). The shapes above remain canonical when the function value is not nil.

A panic that escapes the launched call ends the program with status 2, whether or not the callee is a coroutine, as in Go (see [Program termination](./TRANSLATION_RULES.md#program-termination)).

## Structural rule

Do not translate `go f(x())` as a wrapper whose body evaluates `x()` after launch:

```cpp
// Not canonical: x() runs in the launched goroutine.
go([&] { return f(x()); });
```

That changes observable evaluation timing and can reorder side effects, panics, or mutations relative to the launching goroutine.

The canonical thunk contains only the call of prepared temporaries. It never contains a source expression other than the call itself. A function literal passed directly to `go(...)` is not an exception, because its body is the callee body.

The examples in [`examples/`](../examples) predate this contract and may use other launch shapes.
