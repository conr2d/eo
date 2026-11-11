# Deterministic Translation Rules

This document defines the contract for mechanically translating Go source into Eo-based C++.

The goal is not merely to produce semantically equivalent C++. The translated result should remain structurally aligned with the Go source so that humans and tooling can compare the implementations and reapply upstream Go changes with minimal semantic interpretation.

For translation principles and compatibility scope, see [Translation Principles](./TRANSLATION.md). Construct-specific documents may define additional canonical mappings, such as [Goroutine Translation Design](./GOROUTINE.md) and [Select Translation Design](./SELECT.md).

## Deterministic output

A supported Go construct should have one canonical Eo translation shape.

When multiple C++ forms would preserve behavior, translators should choose the documented canonical form rather than selecting among stylistic alternatives.

This is especially important for LLM-based translation: the same Go structure should produce substantially the same C++ structure across translation runs.

Translation rules should therefore avoid choices based on taste, local refactoring opportunities, or inferred intent that is not represented in the source.

## Preserve source correspondence

Unless C++ or a documented Eo mapping requires otherwise, preserve:

- file-level declaration order;
- function and method boundaries;
- identifier spelling;
- statement order;
- control-flow shape and nesting;
- declaration placement and scope;
- switch/select clause order;
- field order;
- meaningful source comments.

Do not perform unrelated refactoring during mechanical translation.

In particular, do not introduce helper extraction, condition inversion, loop restructuring, early-return rewrites, declaration hoisting, variable merging, expression simplification, or other idiomatic C++ transformations merely because they appear cleaner.

Native C++ redesign and optimization belong in a separate step after a mechanically translated baseline exists.

## Preserve evaluation boundaries

Go evaluation order and evaluation timing are part of the translation contract whenever they are observable.

Do not move an expression across a statement or control-flow boundary unless the corresponding translation rule explicitly permits it.

When C++ requires a temporary to preserve evaluation order, ownership, lifetime, or coroutine behavior, introduce the smallest local temporary needed and keep it adjacent to the source construct it represents.

Do not introduce temporaries solely for stylistic reasons.

## Identifier mapping

Preserve Go identifier spelling whenever it is a valid, non-reserved C++ identifier.

If an identifier must change because of C++ syntax, a reserved identifier, a semantic conflict, or an explicit Eo mapping, use the documented mapping rather than inventing a local rename.

A translator must not rename identifiers merely to follow a C++ naming convention.

## Canonical construct mappings

Only mappings that have been explicitly frozen are canonical.

### Untyped constant arguments

Go evaluates an untyped constant expression exactly, in its own kind, and only then converts its value to the type the context requires. For a call argument, that type is the parameter type, or, for an interface parameter, the constant's default type (`bool`, `rune`, `int`, `float64`, `complex128`, or `string`), which is then converted to the interface.

This rule applies to the argument temporaries of `go` and `defer` statements. The same hazard exists for untyped constants in ordinary calls, assignments, and comparisons; a general rule for those is future work.

Do not translate the source expression of an untyped constant argument. C++ would evaluate it in a fixed-width type with its own operator precedence. Instead, write the folded constant value that the Go type checker reports for the argument (`go/types` `TypeAndValue.Value`) as `T{L}`, where `T` is the translated target type, including a translated defined type such as `type Celsius float64`, and `L` is a C++ literal of the value:

- integer: `L` is the decimal value, with a `u` suffix when the type is unsigned; the minimum value of a signed type is written `-M - 1`, where `M` is the type's maximum;
- floating-point: the recorded value `v` is already rounded to the type. Let `x` be `v` as a Go float (`constant.Float64Val` or `constant.Float32Val`) and `s` be `strconv.FormatFloat(x, 'g', -1, bits)`. If `constant.Compare(constant.MakeFromLiteral(s, token.FLOAT, 0), token.EQL, v)` holds, the decimal `s` is exactly `v`, and `L` is `s`, with `.0` appended when it contains neither `.` nor `e`. Otherwise `L` is the hexadecimal floating literal `strconv.FormatFloat(x, 'x', -1, bits)`, which is always exact. Add an `f` suffix when the underlying Go type is `float32`;
- boolean: `L` is `true` or `false`.

Untyped string and complex constant arguments are not frozen, because no translation of string or complex values is frozen yet. A `go` or `defer` statement with such an argument is provisional. The defer contract never froze a string form, so this does not narrow it.

```go
go f(1 / 3)       // func f(x float64); Go value 0
go f(1.0 / 3)     // func f(x float64)
go f(7 / 2 * 2.0) // func f(x float64); Go value 6
go g(1<<2 + 1)    // func g(n int64); Go value 5
go h(37.5)        // func h(c Celsius); type Celsius float64
go k(0.1)         // func k(x float32)
go f(1e23)        // func f(x float64)
```

```cpp
double _eo_go_arg_0_0 = double{0.0};
double _eo_go_arg_1_0 = double{0x1.5555555555555p-02};
double _eo_go_arg_2_0 = double{6.0};
int64_t _eo_go_arg_3_0 = int64_t{5};
Celsius _eo_go_arg_4_0 = Celsius{37.5};
float _eo_go_arg_5_0 = float{0x1.99999ap-04f};
double _eo_go_arg_6_0 = double{0x1.52d02c7e14af6p+76};
```

Because of the `T{L}` form, the constant's C++ type is always `T`, even when `T` is a defined type whose literals would otherwise have the underlying type. The C++ mapping of Go defined types is not frozen; `T{L}` applies to whatever type it produces. Goroutine temporaries are declared with the parameter's value type, as for any argument. A defer temporary may stay `auto` when `T` is the parameter's value type; for an interface parameter, where `T` is the default type, declare the defer temporary with the parameter's value type, so that the conversion happens at the defer statement.

### Pointers and escaping variables

Go keeps a variable alive for as long as anything can reach it, and a function literal or goroutine that uses a variable shares it with the function that declared it. A C++ local is destroyed at the end of its block, and a lambda that captures by value copies the variable. This mapping gives such a variable shared heap storage that stays alive while any pointer refers to it. The Go compiler, not the translator, decides which variables need it.

#### Pointer type

Go `*T` maps to `ptr<T>` from `<eo/ptr.h>`, which `<eo/core.h>` includes:

| Go | C++ |
| --- | --- |
| `*T` | `ptr<T>` |
| `nil` as a `*T`, and the zero value of `*T` | `ptr<T>{}`, or `nullptr` where it converts |
| `p == nil`, `p != nil` | `p == nullptr`, `p != nullptr` |
| `p == q` | `p == q` |
| `*p` | `*p` |
| `p.f` | `p->f` |
| `new(T)` | `New<T>()` |
| `&T{...}` | `New<T>(...)` with the translated composite literal as the argument |
| `&x`, where `x` is [boxed](#boxed-variables) | `x` |
| `&x.f`, where `x` is boxed | `ptr(x, &x->f)` |
| `&x`, `&x.f`, where `x` is not boxed | `ref(x)`, `ref(x.f)` |
| `&p.f`, where `p` is a pointer | `ptr(p, &p->f)` |
| `&*p` | `p` |
| `&p[i]`, where `p` is a pointer to an array | `ptr(p, &(*p)[i])` |

`&x.f` stands for the address of any part of `x`: `x` followed by field selectors and array indexes that stay inside `x`'s own storage, such as `x.f[i].g`. The `ptr(owner, address)` form keeps the whole target of `owner` alive, as Go does for a pointer into the middle of a variable.

A pointer from `New` owns its target, and every copy shares it. The target is destroyed when the last pointer to it is gone. A pointer from `ref` does not own its target. The rules below use `ref` only for storage that outlives every use of the pointer: package-level variables, and locals that are not boxed.

Pass, return, and store pointers by value, as `ptr<T>`, never as `const ptr<T>&`. A reference to a pointer variable would see a later assignment to that variable, which a Go parameter does not. If that assignment dropped the last owner, the reference would also refer to a destroyed target. Copying an owning pointer increments an atomic count. Copying a `ref` pointer does not.

`*p`, `p->f`, and `ptr(p, &p->f)` on a nil `p` panic with Go's runtime error `runtime error: invalid memory address or nil pointer dereference`. A deferred `recover()` can stop it, as in Go. Otherwise the program ends with status 2. Go also prints a signal line to stderr, which is not part of parity.

A reference cycle of owning pointers is never destroyed, while Go's garbage collector frees it. Eo does not provide weak pointers. The Go source does not record which pointer in a cycle could be weak, and a wrong choice would destroy a target that is still reachable. A program that keeps creating unreachable cycles uses more memory under Eo than under Go, but it behaves the same otherwise (see [#80](https://github.com/conr2d/eo/issues/80)).

Destroying a target releases the pointers it holds without recursion, so dropping the head of a long linked list frees it without overflowing the stack.

A data race on a pointer variable, such as one goroutine assigning `p` while another copies it, is undefined behavior in Eo, as it is for any variable outside the [data-race-free programs](./TRANSLATION.md) that Eo targets. Go happens to keep a racy single-word pointer write memory-safe; a reference-counted pointer does not.

#### Escape facts

Build the package being translated with the Go toolchain that its module selects, and keep the compiler's escape report:

```sh
go build -gcflags='all=-l -m=2' <package>
```

Two kinds of lines are used, each naming a variable by its declaration position:

- `moved to heap: x`: the variable can outlive its function's frame, or is shared with code that can;
- `<function> capturing by value: x` and `<function> capturing by ref: x`: how a function literal captures `x`. Go captures by value only a variable that is never written after it is captured, so the copy always equals the variable.

Ignore capture lines for compiler temporaries, whose names start with `.autotmp_`. A capture line names the function that declares the literal, and the position of the captured variable's declaration.

`-l` turns off inlining. With inlining, the compiler can inline a function literal and drop its capture lines, so the facts would depend on its inlining decisions. Without inlining, a few variables may be moved to heap that an inlining build keeps on the stack. Boxing them costs an allocation, and the translation stays correct.

#### Boxed variables

A local variable or parameter `x` is boxed if:

1. the escape facts report `moved to heap: x`; or
2. `x`'s address is taken, or a function literal captures `x` by reference, and `x` is declared anywhere except directly in the function body before the `try` block of the [defer shape](#defer), in a loop's own clause, or directly in a loop body. This includes the body of an `if`, `else`, `switch` case, or `select` case, a bare block `{ ... }`, the init statement of an `if` or `switch`, a type switch guard, a `select` receive such as `case v := <-ch:`, and the top level of the `try` block, at any depth.

A variable declared by a loop's own clause or directly in a loop body is not boxed by rule 2. Go reports a variable moved to heap when its address can outlive one iteration, so any other pointer to it is declared inside the same iteration and is destroyed with it.

Taking the address of `x` means `&x` or `&x.f` on a part of `x`, a call or method value of a pointer-receiver method on `x` or a part of `x` (`x.Lock()`, `x.f.Add(1)`, `x.M`), and a slice expression on `x` or a part of `x` that is an array.

Rule 2 is needed because Go keeps a variable that is not moved to heap in its function's frame until the function returns, while the C++ variable is destroyed at the end of its block:

```go
var p *int
{
	x := 5
	p = &x // Go keeps x on the stack, but p still refers to it below
}
*p += 1
```

```cpp
ptr<int64_t> p;
{
  auto x = New<int64_t>(5);
  p = x;
}
*p += 1;
```

A boxed variable is declared as a pointer to its value, with the translated Go type of the variable as `T`:

| Go | C++ |
| --- | --- |
| `var x T` | `auto x = New<T>();` |
| `x := v`, `var x T = v` | `auto x = New<T>(v);` |
| parameter `x T` | parameter `T _eo_param_x`, and `auto x = New<T>(_eo_param_x);` at the start of the body |
| named result `x T` | `auto x = New<T>();` at the start of the body, in place of the `T x{}` of the defer shape, and `*x` wherever the function returns `x` |

At the start of the body, boxed parameters come first, in parameter order, then result storage, then `eo_defer_scope`.

Every other use of `x` is `*x`, `x.f` is `x->f`, and `&x` is `x`.

#### Loop clause variables

A variable that the init statement of a three-clause `for` loop declares, and that is moved to heap, is boxed in the init statement. Go gives every iteration its own variable: before the post statement runs, it declares the next iteration's variable and copies the current value into it. The C++ post expression therefore first assigns a new box to each boxed clause variable, in declaration order, and then runs the translated post statement, joined by the comma operator:

```go
var head *Node
for i := 0; i < 3; i++ {
	head = &Node{p: &i, next: head}
}
```

```cpp
ptr<Node> head;
for (auto i = New<int64_t>(0); *i < 3; i = New<int64_t>(*i), (*i)++) {
  head = New<Node>(Node{.p = i, .next = head});
}
```

Each node keeps the box of its own iteration, so the values are 0, 1, and 2, as in Go. The condition and the post statement act on the new box. `continue` jumps to the C++ post expression, so the copy happens there too, as it does in Go. A loop without a post statement, `for i := 0; i < n; {`, has only the new-box assignments as its C++ post expression.

A clause variable that is not boxed stays a plain C++ variable that every iteration reuses. Nothing that outlives an iteration refers to it, so the per-iteration copy cannot be observed.

An init statement that declares variables of different translated types and a post statement that does not translate to a single C++ expression, such as `i, j = i+1, j-1`, are not covered. A variable that a `range` clause declares is not covered either, because `range` loops have no mapping yet (see [#84](https://github.com/conr2d/eo/issues/84)).

#### Function literals

A function literal translates to a lambda `[=, &a, &b]() mutable`, where `a` and `b` are the variables that the literal captures by reference and that are not boxed. Every other captured variable is copied. A variable that Go captures by value never changes after it is captured, so the copy is equivalent. A copied box is a copy of the pointer, so it shares the variable. Package-level variables are not captured.

```go
x := 0 // captured by reference, not moved to heap
y := 5 // captured by value
inc := func() { x++ }
get := func() int { return x + y }
```

```cpp
auto x = int64_t{0};
auto y = int64_t{5};
auto inc = [=, &x]() mutable { x++; };
auto get = [=, &x]() mutable -> int64_t { return x + y; };
```

By rule 2, a variable captured by reference that is not boxed is declared directly in the function body before any `try` block, in a loop clause, or directly in a loop body. It is alive for every call the literal can receive: Go reports the variable moved to heap if the literal can be called after the function returns, or after the iteration that declared the variable.

Two kinds of literal have their own frozen shape:

- a literal launched by `go` is `[=]() mutable`, as described in [Goroutine](./GOROUTINE.md#function-literals). A goroutine's literal always outlives its frame, so every variable that it captures by reference is moved to heap;
- a deferred literal is `[&]` when every variable it captures is a parameter, result, or variable declared before the `try` block of its function, or a variable of an enclosing function, which the function reaches through its own capture for as long as it runs. Otherwise it is `[&, v...]() mutable`, listing by copy each captured variable that is declared inside the `try` block. Such a variable is captured by value, is boxed by rule 1 or rule 2, or is declared in a loop clause or directly in a loop body, where Go moves it to heap if a deferred literal captures it by reference (see [Defer](#defer)). A boxed loop clause variable is copied as the box of the iteration that registers the call, as in `eo_defer([&, i]() mutable { use(*i); });`.

#### Methods

A method of a translated type `T` is a static member function whose first parameter is the receiver: a `ptr<T>` for a pointer receiver, and a `T` for a value receiver. A value receiver is then an ordinary parameter, which is boxed by the same rules as any other:

```go
func (c *Counter) Start() {
	go c.loop()
}
```

```cpp
struct Counter {
  // fields
  static void Start(ptr<Counter> c);
  static auto loop(ptr<Counter> c) -> func<>;
};

void Counter::Start(ptr<Counter> c) {
  auto _eo_go_receiver_0 = c;
  go([=]() mutable { return Counter::loop(_eo_go_receiver_0); });
}
```

The receiver is not `this`, because a method can keep its receiver after it returns, here in a goroutine, and `this` cannot keep the target alive. A value receiver is not `*this` either: a goroutine or closure that captures it must capture a copy or a box, never `this`.

| Go call | Pointer receiver | Value receiver |
| --- | --- | --- |
| `p.M(args)`, `p` a pointer | `T::M(p, args)` | `T::M(*p, args)` |
| `x.M(args)`, `x` addressable | `T::M(&x, args)`, with `&x` translated as above | `T::M(x, args)` |

Go does not specify whether `*p` in a value-receiver call is evaluated before or after the other arguments, and C++ does not either, so `T::M(*p, f())` is translated as written. As in Go, a pointer-receiver call on a nil pointer passes nil, and only a dereference inside the method panics, while a value-receiver call through a nil pointer panics at the call.

Methods of Eo library types, such as `sync::Mutex` and `sync::WaitGroup`, stay member functions, called as `x.M()` or `p->M()`.

#### Not covered yet

- pointers held inside slices, maps, strings, and interfaces, whose own mappings are not frozen, and the C++ type of a slice of an array;
- printing a pointer with `fmt`, which shows an address;
- method values and method expressions, promoted methods of embedded fields, and conversions through `unsafe.Pointer` or `uintptr`.

### Goroutine

Go `go` statements follow the canonical mapping defined in [Goroutine Translation Design](./GOROUTINE.md).

In particular:

- evaluate and save a function-valued callee before its arguments;
- evaluate call arguments in source order in the launching goroutine;
- preserve Go value- vs pointer-receiver binding for method calls;
- keep generated evaluation temporaries adjacent to the source `go` statement;
- declare each argument temporary with the translated parameter's value type (reference and `const`/`volatile` removed), not `auto`; a launch whose translated parameter is a non-owning view type is provisional;
- launch a nullary `[=]() mutable` thunk whose body is only the call of the prepared temporaries, so the callee is invoked after launch even if it is not a lazy coroutine;
- pass a function literal with no parameters, called without arguments, to `go(...)` directly, since its body is the callee body, and save any other function literal as the function value;
- do not move any other source expression into the launched thunk;
- translate addresses, pointers, and captures as described in [Pointers and escaping variables](#pointers-and-escaping-variables), which boxes every variable that a goroutine can reach, so that no launch needs a lifetime check of its own (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary)); a launch that passes a value referring to storage through a type whose mapping is not frozen, such as a slice of a local array, is provisional;
- call a method of a translated type as `T::M(receiver, args...)` with the saved receiver;
- apply Go 1.22 and later per-iteration loop-variable semantics; a loop's own post statement or `range` update is not a write to the variables that its clause declares with `:=`, but it is a write to any other variable.

`go(...)` returns without running the launched call, including when it is called from a running goroutine.

The launched function body begins only after the source `go` statement's function and argument evaluation is complete.

### Select

Go `select` follows the canonical mapping defined in [Select Translation Design](./SELECT.md).

In particular:

- a blocking select uses `co_await select.index()`;
- a select with `default` uses `select.try_index()`;
- the `Select` object is constructed for one execution of one source select;
- receive results are accessed synchronously from the selected case;
- communication clauses keep source order;
- every translated case body is braced;
- communication commits before the selected case body executes.

Do not use an alternate Select shape merely because it is equivalent for a particular program.

### Defer

The following mapping is canonical for defer registration, function returns, panics that run deferred calls, and `recover`.

A translated function containing at least one Go `defer` statement declares one function-scoped defer stack at the beginning of the C++ function body:

```cpp
func<> f() {
  eo_defer_scope;
  // ...
}
```

The stack stores deferred calls but does not execute them from its destructor. Every translated normal exit explicitly executes `eo_defer_run` before leaving the function. This keeps function-scope C++ locals alive while deferred calls execute.

A panic is a C++ exception, so the function also catches it and runs its deferred calls with `eo_defer_panic` (see [Panic and recover](#panic-and-recover)). The full shape of a function with a `defer` statement is:

```cpp
auto f(int x) -> int {
  int _eo_result_0{};
  eo_defer_scope;
  // statements before the first statement that contains a defer
  try {
    // the first statement that contains a defer, and every statement after it
  } catch (...) {
    eo_defer_panic;
  }
  return _eo_result_0;
}
```

- Each result has storage declared before `eo_defer_scope`: a named result uses its translated name, and an unnamed result `N` uses `_eo_result_N`. Both are declared as `T name{}` with the translated result type. A function with more than one result is provisional, as multi-value results have no frozen mapping.
- `try {` opens immediately before the first statement at the top level of the function body that is, or contains, a `defer` statement, and before that statement's argument temporaries. It closes at the end of the function body. A panic before that point has no deferred call to run and leaves the function directly. A `defer` inside a nested function literal belongs to that literal, which gets this shape itself, and does not count for the enclosing function.
- After the catch handler, the function returns its result storage: `return _eo_result_0;`, `co_return _eo_result_0;`, or the named results. A function without results ends with nothing, or with `co_return;` for a coroutine. This return is reached only when a deferred call recovered the panic.

A direct named-function defer evaluates and saves its argument expressions in source order before registration. Temporary names use the source defer ordinal followed by the argument ordinal. Defer statements are counted like `go` statements, as described in [Temporary names](./GOROUTINE.md#temporary-names), with a separate count:

```go
defer cleanup(x(), y())
```

```cpp
auto _eo_defer_arg_0_0 = x();
auto _eo_defer_arg_0_1 = y();
eo_defer([=] { cleanup(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
```

Defer argument temporaries are still declared with `auto`, unlike goroutine argument temporaries, which use the translated parameter type (see [Argument temporary types](./GOROUTINE.md#argument-temporary-types)). An untyped constant argument follows [Untyped constant arguments](#untyped-constant-arguments). Until the defer shape is aligned, any other defer argument whose Go conversion changes its type, such as `nil` or a non-constant value passed to an interface parameter, is provisional.

If the deferred callee is itself a function-valued expression, evaluate and save it before the arguments:

```go
defer next()(x())
```

```cpp
auto _eo_defer_fn_0 = next();
auto _eo_defer_arg_0_0 = x();
eo_defer([=]() mutable { _eo_defer_fn_0(_eo_defer_arg_0_0); });
```

A deferred method call saves the receiver binding before its arguments. The receiver temporary must reflect the Go method receiver kind, including Go's implicit address/dereference rules.

For a value-receiver method, save the receiver value that Go would pass to the method. This includes dereferencing a pointer receiver expression when Go selects a value-receiver method through a pointer:

```cpp
auto _eo_defer_receiver_0 = x;
auto _eo_defer_arg_0_0 = y();
eo_defer([=]() mutable { T::M(_eo_defer_receiver_0, _eo_defer_arg_0_0); });
```

For a pointer-receiver method invoked on an addressable value, preserve Go's implicit `&x` binding by saving a pointer to the original value rather than copying it. The pointer is `&x` translated as described in [Pointers and escaping variables](#pointers-and-escaping-variables), and the call uses the [method form](#methods):

```go
defer x.M(y()) // M has receiver *T, and x is not boxed
```

```cpp
auto _eo_defer_receiver_0 = ref(x);
auto _eo_defer_arg_0_0 = y();
eo_defer([=] { T::M(_eo_defer_receiver_0, _eo_defer_arg_0_0); });
```

If `x` is boxed, the receiver is `x` itself: `auto _eo_defer_receiver_0 = x;`. If the source receiver expression already evaluates to a pointer for a pointer-receiver method, evaluate and save that pointer value directly. Translators must resolve the Go receiver kind from the method declaration instead of inferring it from convenient C++ syntax.

A method of an Eo library type, such as `sync::Mutex`, is a member function. Its receiver binding saves a raw `&x` for a package-level `x` or a variable declared before the `try` block, and otherwise `&x` translated by the pointer table, such as the box for a boxed `x` or `ptr(c, &c->mu)` for `defer c.mu.Unlock()`. The deferred call is `_eo_defer_receiver_0->M(...)`, as in [Mutex](#mutex).

A deferred closure may use a direct reference capture only when every referenced translated object remains alive until the function-level `eo_defer_run` and `eo_defer_panic`:

```go
defer func() { use(x) }()
```

```cpp
eo_defer([&] { use(x); });
```

Parameters, result storage, package-level variables, and locals declared at the top level of the function body before `try {` stay alive for both. A local declared inside the `try` block, including one in an inner block or a loop iteration, is destroyed before the catch handler runs `eo_defer_panic`, and an inner-block local before the drain at a later return. A deferred literal that captures such a local lists it by copy, as in `eo_defer([&, v]() mutable { use(v); });`. The local is captured by value in Go, or it is boxed (see [Function literals](#function-literals)), so the copy keeps Go's semantics. A saved address of such a local, such as a pointer-receiver binding, is the box itself: rule 2 of [Boxed variables](#boxed-variables) boxes a local at the top level of the `try` block or in an inner block, and Go moves a loop-body local to heap when a deferred call keeps its address.

Repeated execution, including inside loops, registers a new callable each time. Registered calls execute in LIFO order when `eo_defer_run` executes. The `eo_defer_scope` declaration belongs to the surrounding translated function, not to a nested block containing the defer statement.

For a void normal return, drain immediately before the return:

```cpp
eo_defer_run;
return;
```

For a value return, evaluate the return expression into the result storage first, then drain, then return the storage:

```go
return result()
```

```cpp
_eo_result_0 = result();
eo_defer_run;
return _eo_result_0;
```

A bare `return` with named results drains and returns the named results. A deferred call may change a named result, because it is still the storage that the function returns.

Coroutine returns follow the same rule:

```cpp
eo_defer_run;
co_return;
```

A function that reaches its end without an explicit return inserts `eo_defer_run` at the natural function exit. Every early normal return must have its own corresponding drain sequence.

### Panic and recover

`panic(v)` translates to `panic(v)` from `<eo/panic.h>`, which throws `eo::Panic`. Its `what()` is `panic: ` followed by the value as Go prints it, and `value()` holds the value. A Go string panic value is stored as `std::string`. Runtime panics raised by Eo are `eo::Panic` too: the channel panics such as `close of closed channel`, `sync: negative WaitGroup counter`, `cannot create context from nil parent`, and `invalid argument to Intn`. Any other C++ exception that leaves a translated call is also a panic. `panic(nil)` translates to `panic(nullptr)` and, as since Go 1.21, raises a panic whose message is `panic called with nil argument`.

While a panic leaves a function that contains a `defer` statement, its catch handler runs `eo_defer_panic`:

- the remaining deferred calls run in LIFO order, after those of the functions it called and before those of its callers;
- a deferred call that panics replaces the running panic, and the remaining deferred calls still run;
- if the last panic was recovered, the function returns its result storage normally, and its caller continues. Otherwise the panic leaves the function.

A panic in a deferred call during `eo_defer_run` leaves `eo_defer_run`, and the same catch handler runs the remaining deferred calls.

`recover()` translates to `recover()`, which returns an `eo::Recovered`:

- `r := recover()` translates to `auto r = recover();`;
- `r == nil` and `r != nil` translate to `r == nullptr` and `r != nullptr`;
- printing `r` with `fmt::Println` prints the panic value as Go does, and `<nil>` for nil;
- `panic(r)` starts a new panic with the recovered value and message, and `panic(r)` with a nil `r` is `panic(nil)`. For a panic raised by a C++ exception other than `eo::Panic`, the new panic keeps the message, and its value is empty.

`recover()` returns the running panic, and stops it, only when it is called by a deferred call that `eo_defer_panic` runs. It returns nil when no panic is running, in a deferred call run by `eo_defer_run`, and after the panic was recovered. Go also returns nil when recover is not called directly by the deferred function. Eo cannot detect that. A `recover()` is frozen when it is in one of these places:

- directly in the body of a deferred function literal, `defer func() { ... recover() ... }()`, but not in a function literal nested in that body;
- directly in the body of a named function or method that the program calls only in `defer` statements.

Any other `recover()` is unsupported, because a call from inside a deferred function would recover where Go returns nil.

The type of a recovered value is provisional beyond comparison with nil and printing: `r.value()` is a `std::any`, and a type assertion such as `r.(error)` has no mapping. `runtime.Goexit` has no mapping.

### Blocking operations

A Go operation that blocks the calling goroutine translates to an awaitable that the translation suspends on with `co_await`. It never blocks the OS thread. A blocked worker thread stops other goroutines from running, and with `EOMAXPROCS=1` it stops all of them, while Go parks only the goroutine.

| Go | C++ | Status |
| --- | --- | --- |
| `select` without `default` | `co_await select.index()` | frozen, see [Select](#select) |
| `wg.Wait()` | `co_await wg.Wait()` | frozen |
| `mu.Lock()` | `co_await mu.Lock()` | frozen, see [Mutex](#mutex) |
| `rw.Lock()`, `rw.RLock()` | `co_await rw.Lock()`, `co_await rw.RLock()` | frozen, see [Mutex](#mutex) |
| `ch <- v` | `co_await (ch << v)` | provisional |
| `v := <-ch` | `auto v = co_await *ch` | provisional |
| `time.Sleep(d)` | `co_await time::Sleep(d)` | provisional, including the duration |

A Go function whose body contains a blocking operation, or a call to a function translated as a coroutine, is translated as a coroutine returning `func<T>`, and every call to it is `co_await`ed. Other functions stay plain C++ functions. Calls through function values and interface methods, whose target is not known statically, are provisional.

Two call sites do not `co_await`:

- the `go` thunk returns the call (`return f(...)`), and `go(...)` runs the returned awaitable as the goroutine body (see [Goroutine](#goroutine));
- a deferred call runs synchronously at `eo_defer_run`, so it cannot suspend. A `defer` is unsupported when its call is a blocking operation or a coroutine, such as `defer wg.Wait()`, and when it defers a function literal whose body would be a coroutine, such as `defer func() { ch <- v }()`: the registered call would create an awaitable and drop it, and the deferred work would never run.

Do not translate a Go lock to `std::mutex` or `std::shared_mutex`. Their `lock()` blocks the OS thread, and a translation can then deadlock where Go does not: with `EOMAXPROCS=1`, or whenever the holder is suspended at a `co_await` while it holds the lock.

`sync.Once` and `sync.Cond` have no mapping yet. `Once.Do` blocks while another goroutine runs the function, and `Cond.Wait` blocks until a signal, so `std::call_once` and `std::condition_variable` would block the worker. A translation that uses either is unsupported (see [#76](https://github.com/conr2d/eo/issues/76)).

Do not use `eo::invoke` in a translation. It blocks the calling OS thread until a coroutine finishes, and it exists for C++ code outside the runtime, such as tests.

`sync.WaitGroup` maps to `sync::WaitGroup` from `<eo/sync.h>`. `Add` and `Done` are plain calls, and `Add` panics with `sync: negative WaitGroup counter` when the counter goes below zero.

Sharing a WaitGroup with goroutines follows [Pointers and escaping variables](#pointers-and-escaping-variables). A local `var wg sync.WaitGroup` that goroutines use through a capture or `&wg` is moved to heap, so it is boxed as `auto wg = New<sync::WaitGroup>();` and used as `wg->Add(1)` and `co_await wg->Wait()`. A `*sync.WaitGroup` is a `ptr<sync::WaitGroup>`. The `sync::WaitGroupPtr` alias for `std::shared_ptr<sync::WaitGroup>` is not a translation mapping.

#### Mutex

`sync.Mutex` and `sync.RWMutex` map to `sync::Mutex` and `sync::RWMutex` from `<eo/sync.h>`:

| Go | C++ |
| --- | --- |
| `mu.Lock()` | `co_await mu.Lock();` |
| `mu.Unlock()` | `mu.Unlock();` |
| `mu.TryLock()` | `mu.TryLock()` |
| `rw.RLock()` | `co_await rw.RLock();` |
| `rw.RUnlock()` | `rw.RUnlock();` |
| `rw.TryRLock()` | `rw.TryRLock()` |

`RWMutex` also has `Lock`, `Unlock`, and `TryLock`, mapped as for `Mutex`. Only `Lock` and `RLock` are blocking operations. The others are plain calls, so a `defer` of `Unlock` or `RUnlock` is allowed. As in Go, a goroutine waiting in `rw.Lock()` keeps new readers out until it has unlocked.

Unlocking a lock that is not held in that mode is a fatal error, as in Go. The runtime writes `fatal error: ` and the message to stderr, and exits with status 2:

- `sync: unlock of unlocked mutex`;
- `sync: Unlock of unlocked RWMutex`;
- `sync: RUnlock of unlocked RWMutex`.

Deferred calls do not run, and `recover()` cannot stop a fatal error. When the fatal error happens in a deferred call during a panic, Go also prints that panic first and Eo does not. Stderr is not part of parity, as with panic reports.

`mu.Lock(); defer mu.Unlock()` follows [Defer](#defer). `Unlock` has a pointer receiver, so the defer saves `&mu`. `co_await mu.Lock()` comes before `try {`, because it does not contain the defer:

```go
var mu sync.Mutex
var count int

func inc() {
	mu.Lock()
	defer mu.Unlock()
	count++
}
```

```cpp
func<> inc() {
  eo_defer_scope;
  co_await mu.Lock();
  try {
    auto* _eo_defer_receiver_0 = &mu;
    eo_defer([=] { _eo_defer_receiver_0->Unlock(); });
    count++;
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
```

`inc` contains a blocking operation, so it is a coroutine, and its callers `co_await inc()`.

Holding a lock across a blocking operation, such as a channel send, needs nothing extra: goroutines waiting in `Lock` are suspended, so the holder can still run.

The following are not frozen:

- the C++ form of package-level variables and struct fields, as used in `tests/parity/mutex_counter`. Sharing a local Mutex or RWMutex follows the WaitGroup rules above and is frozen, as in `tests/parity/mutex_local_shared`;
- `RLocker`, and passing a lock as a `sync.Locker`, have no mapping;
- `sync::Mutex`, `sync::RWMutex`, and `sync::WaitGroup` cannot be copied or moved. A Go program that copies one, which `go vet` reports, is unsupported.

### Program termination

A translated program links `eo::main` and defines `func<> eo_main()` in place of Go's `main`. The runtime ends the process the way Go does:

- when `eo_main` completes, the process exits with status 0 at once, without waiting for other goroutines. If another goroutine has an `eo::Panic` that is not yet recovered, the runtime first waits up to about 100 ms for it, as Go's `runtime.main` yields to goroutines running deferred calls for a panic. Another C++ exception counts only while one function runs its deferred calls for it, not while it passes between functions. If a panic is then being reported, it ends the process with status 2 instead;
- `os.Exit(code)` translates to `os::Exit(code)` from `<eo/os.h>`. It ends the process at once from any goroutine, and deferred calls do not run. Do not translate it to `std::exit`: that runs static destructors, and the runtime's thread pool destructor then waits forever for its worker threads;
- an exception that escapes `eo_main`, or the call launched by `go(...)`, is an unrecovered panic. The panicking goroutine's deferred calls have already run in its catch handlers (see [Panic and recover](#panic-and-recover)). The runtime reports the panic on stderr and exits with status 2;
- `fmt::Println` and `fmt::Fprintln` write and flush each call, as Go writes unbuffered to `os.Stdout` and `os.Stderr`. Output is not lost when the process is killed or exits.

Each exit flushes `std::cout`, `stdout` and `stderr`, then ends the process with `std::_Exit`. A stream whose lock another thread holds is skipped, for example while a goroutine is blocked writing to a pipe that nobody reads; that write is lost, as it would be in Go. Other `FILE` streams are not flushed, and no static destructors run.

With `EOMAXPROCS=1`, a goroutine blocked in a read or write occupies the only worker thread, so the goroutine that would call `os::Exit` or return from `eo_main` never runs. Go hands the processor to another thread during a blocking system call. See [#71](https://github.com/conr2d/eo/issues/71).

## Unfrozen constructs

If no canonical translation rule exists for a Go construct, do not silently invent a project-wide convention and treat it as stable.

A translator may use a local adaptation when necessary to make progress, but it should be explicit that the mapping is provisional and may need to change when the translation contract is extended.

Translation-facing mappings with broad source impact should be frozen before large-scale porting depends on them.

Examples currently requiring additional canonical rules include:

- goroutine launches that pass a non-owning view argument, or a value that refers to storage through a type whose mapping is not frozen, such as a slice of a local array (see [Lifetime boundary](./GOROUTINE.md#lifetime-boundary));
- pointers held in slices, maps, strings, and interfaces, printing a pointer, method values, method expressions, and promoted methods (see [Not covered yet](#not-covered-yet));
- untyped string and complex constant arguments of `go` and `defer`;
- goroutine launches of method expressions, interface method calls, variadic calls, and multi-value calls, and launches with a nil function value (see [Unfrozen launch forms](./GOROUTINE.md#unfrozen-launch-forms));
- the type of a recovered panic value beyond comparison with nil and printing, and `runtime.Goexit` (see [Panic and recover](#panic-and-recover));
- labeled `break` and `continue`;
- `range` loops, including a boxed variable that a `range` clause declares (see [#84](https://github.com/conr2d/eo/issues/84)), and the three-clause loop forms that [Loop clause variables](#loop-clause-variables) does not cover;
- general zero-value construction for translated Go types;
- panic-producing operations whose naive C++ equivalent would throw differently or invoke undefined behavior.

## Unsupported constructs

Unsupported behavior should fail visibly rather than being approximated silently.

A translator should not replace an unsupported Go construct with a convenient C++ behavior that changes observable semantics.

When a translation cannot be completed under the current Eo contract, the unresolved construct should remain easy to locate and should identify the missing translation rule.

The purpose of this rule is to keep translation gaps explicit so they can drive Eo development instead of becoming hidden semantic drift.

Currently unsupported:

- `defer` of a blocking operation or of a call to a function translated as a coroutine (see [Blocking operations](#blocking-operations));
- `recover()` outside a deferred function literal, unless it is in a named function that is only called in `defer` statements (see [Panic and recover](#panic-and-recover)).
- `sync.Once` and `sync.Cond` (see [Blocking operations](#blocking-operations)), `RWMutex.RLocker`, and passing a lock as a `sync.Locker` (see [Mutex](#mutex));
- copying a `sync.Mutex`, `sync.RWMutex`, or `sync.WaitGroup` (see [Mutex](#mutex)).

## Translation review

A mechanical translation should be reviewable against the Go source primarily by structural comparison.

Review should ask:

1. Does each translated region have an obvious source counterpart?
2. Were source order and control-flow boundaries preserved unless a documented rule required a change?
3. Were expressions evaluated at the same observable time and frequency?
4. Were identifier changes limited to documented cases?
5. Were unsupported semantics made explicit instead of approximated?
6. Were native optimizations kept separate from the mechanical port?

A translation that is semantically correct but unnecessarily restructures the source is not the preferred Eo translation.

Structural review is complemented by behavioral checks. A [parity case](../tests/parity/README.md) runs a Go program and its canonical translation against the same expected output, and records a known divergence as a reproducible test rather than leaving it to review.

## Upstream synchronization

Upstream Go changes should normally be applied by locating the corresponding translated region and reproducing the same structural change under the canonical Eo mappings.

The mechanical C++ baseline should therefore remain close enough to the source that an upstream diff is useful as a guide for the C++ update.

When a native C++ rewrite intentionally diverges from the mechanical structure, that divergence should remain isolated so that upstream semantic changes can still be identified and reconciled deliberately.
