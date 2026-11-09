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

Do not translate the source expression of an untyped constant argument. C++ would evaluate it in a fixed-width type with its own operator precedence. Instead, write the folded constant value that the Go type checker reports for the argument (`go/types` `TypeAndValue.Value`) as a C++ literal of the target type `T`:

- an integer type: `T{N}`, where `N` is the decimal value, with a `u` suffix when `T` is unsigned; the minimum value of a signed type is written `T{-M - 1}`, where `M` is the type's maximum;
- a floating-point type: when `T` represents the value exactly (`constant.Float64Val` or `constant.Float32Val` reports it exact), its exact decimal value with a decimal point; otherwise the hexadecimal floating literal of the value rounded to `T` (`strconv.FormatFloat(v, 'x', -1, bits)`). Add an `f` suffix when the Go type is `float32`;
- a boolean: `true` or `false`.

String and complex constants are not frozen yet, because their translated types are not.

```go
go f(1 / 3)       // func f(x float64); Go value 0
go f(1.0 / 3)     // func f(x float64)
go f(7 / 2 * 2.0) // func f(x float64); Go value 6
go g(1<<2 + 1)    // func g(n int64); Go value 5
```

```cpp
double _eo_go_arg_0_0 = 0.0;
double _eo_go_arg_1_0 = 0x1.5555555555555p-02;
double _eo_go_arg_2_0 = 6.0;
int64_t _eo_go_arg_3_0 = int64_t{5};
```

The literal's C++ type is always `T`. Goroutine temporaries are declared with the parameter's value type, as for any argument. A defer temporary may stay `auto` when `T` is the parameter's value type; for an interface parameter, where `T` is the default type, declare the defer temporary with the parameter's value type, so that the conversion happens at the defer statement.

### Goroutine

Go `go` statements follow the canonical mapping defined in [Goroutine Translation Design](./GOROUTINE.md).

In particular:

- evaluate and save a function-valued callee before its arguments;
- evaluate call arguments in source order in the launching goroutine;
- preserve Go value- vs pointer-receiver binding for method calls;
- keep generated evaluation temporaries adjacent to the source `go` statement;
- declare each argument temporary with the translated parameter's value type (reference and `const`/`volatile` removed), not `auto`;
- launch a nullary `[=]() mutable` thunk whose body is only the call of the prepared temporaries, so the callee is invoked after launch even if it is not a lazy coroutine;
- pass a function literal with no parameters, called without arguments, to `go(...)` directly, since its body is the callee body, and save any other function literal as the function value;
- do not move any other source expression into the launched thunk;
- do not take the address of, or capture by reference, a local variable or parameter of the launching function;
- do not capture by value, in a launched function literal, a local that is written after the `go` statement as defined in [By-value captures](./GOROUTINE.md#by-value-captures); escaping and shared locals are not frozen;
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

The following mapping is canonical for defer registration and normal function returns. Named-result mutation, panic/recover, and deferred closures whose captured variables require lifetime extension beyond their translated C++ lexical scope remain unfrozen as described below.

A translated function containing at least one Go `defer` statement declares one function-scoped defer stack at the beginning of the C++ function body:

```cpp
func<> f() {
  eo_defer_scope;
  // ...
}
```

The stack stores deferred calls but does not execute them from its destructor. Every translated normal exit explicitly executes `eo_defer_run` before leaving the function. This keeps function-scope C++ locals alive while deferred calls execute.

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
eo_defer([=]() mutable { _eo_defer_receiver_0.M(_eo_defer_arg_0_0); });
```

For a pointer-receiver method invoked on an addressable value, preserve Go's implicit `&x` binding by saving a pointer to the original value rather than copying it:

```go
defer x.M(y()) // M has receiver *T
```

```cpp
auto* _eo_defer_receiver_0 = &x;
auto _eo_defer_arg_0_0 = y();
eo_defer([=] { _eo_defer_receiver_0->M(_eo_defer_arg_0_0); });
```

If the source receiver expression already evaluates to a pointer for a pointer-receiver method, evaluate and save that pointer value directly. Translators must resolve the Go receiver kind from the method declaration instead of inferring it from convenient C++ syntax.

A deferred closure may use a direct reference capture only when every referenced translated object remains alive until the function-level `eo_defer_run`:

```go
defer func() { use(x) }()
```

```cpp
eo_defer([&] { use(x); });
```

This shape is not valid when the Go closure captures a variable whose translated C++ lexical lifetime ends before the function-level drain, such as a variable declared inside an inner block or a loop iteration. Go may keep such captured variables alive, while a C++ reference capture would dangle. Until Eo defines a canonical escape/capture-cell mapping, translators must leave that case explicitly unsupported or unfrozen rather than emit `[&]` and silently change semantics.

Repeated execution, including inside loops, registers a new callable each time. Registered calls execute in LIFO order when `eo_defer_run` executes. The `eo_defer_scope` declaration belongs to the surrounding translated function, not to a nested block containing the defer statement.

For a void normal return, drain immediately before the return:

```cpp
eo_defer_run;
return;
```

For a value return, evaluate the return expression first, then drain, then return the saved value:

```go
return result()
```

```cpp
auto _eo_return_0 = result();
eo_defer_run;
return _eo_return_0;
```

Coroutine returns follow the same rule:

```cpp
eo_defer_run;
co_return;
```

A function that reaches its end without an explicit return inserts `eo_defer_run` at the natural function exit. Every early normal return must have its own corresponding drain sequence.

Named-result mutation by deferred calls requires explicit result storage that remains mutable until after defer execution and is not yet part of the frozen mapping. Panic/recover and exception-driven exits likewise require separate panic machinery; `eo_defer_run` currently defines the supported normal-return path only.

## Unfrozen constructs

If no canonical translation rule exists for a Go construct, do not silently invent a project-wide convention and treat it as stable.

A translator may use a local adaptation when necessary to make progress, but it should be explicit that the mapping is provisional and may need to change when the translation contract is extended.

Translation-facing mappings with broad source impact should be frozen before large-scale porting depends on them.

Examples currently requiring additional canonical rules include:

- deferred closures whose captured variables outlive their translated C++ lexical scope;
- goroutine calls and function literals that take the address of, capture by reference, or share a written-after-launch local variable or parameter of the launching function;
- goroutine launches of method expressions, interface method calls, variadic calls, and multi-value calls, and launches with a nil function value or a nil value-receiver pointer (see [Unfrozen launch forms](./GOROUTINE.md#unfrozen-launch-forms));
- panics in launched goroutines, which are currently discarded or propagate out of the executor instead of terminating the program (see [Unfrozen launch forms](./GOROUTINE.md#unfrozen-launch-forms));
- labeled `break` and `continue`;
- general zero-value construction for translated Go types;
- panic-producing operations whose naive C++ equivalent would throw differently or invoke undefined behavior.

## Unsupported constructs

Unsupported behavior should fail visibly rather than being approximated silently.

A translator should not replace an unsupported Go construct with a convenient C++ behavior that changes observable semantics.

When a translation cannot be completed under the current Eo contract, the unresolved construct should remain easy to locate and should identify the missing translation rule.

The purpose of this rule is to keep translation gaps explicit so they can drive Eo development instead of becoming hidden semantic drift.

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

## Upstream synchronization

Upstream Go changes should normally be applied by locating the corresponding translated region and reproducing the same structural change under the canonical Eo mappings.

The mechanical C++ baseline should therefore remain close enough to the source that an upstream diff is useful as a guide for the C++ update.

When a native C++ rewrite intentionally diverges from the mechanical structure, that divergence should remain isolated so that upstream semantic changes can still be identified and reconciled deliberately.
