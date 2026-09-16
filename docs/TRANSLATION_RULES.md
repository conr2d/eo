# Deterministic Translation Rules

This document defines the contract for mechanically translating Go source into Eo-based C++.

The goal is not merely to produce semantically equivalent C++. The translated result should remain structurally aligned with the Go source so that humans and tooling can compare the implementations and reapply upstream Go changes with minimal semantic interpretation.

For translation principles and compatibility scope, see [Translation Principles](./TRANSLATION.md). Construct-specific documents may define additional canonical mappings, such as [Select Translation Design](./SELECT.md).

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

A translated function that contains at least one Go `defer` statement declares one function-scoped defer stack at the beginning of the C++ function body:

```cpp
func<> f() {
  eo_defer_scope;
  // ...
}
```

Each direct Go defer call preserves the source call shape through `eo_defer`:

```go
defer cleanup(x())
```

```cpp
eo_defer(cleanup, x());
```

The deferred function value and direct-call arguments are evaluated when `eo_defer(...)` executes. The call itself executes when the surrounding function exits. Repeated execution, including inside loops, registers a new call each time, and registered calls execute in LIFO order.

A deferred closure is registered as a callable value:

```go
defer func() { use(x) }()
```

```cpp
eo_defer([&] { use(x); });
```

The `eo_defer_scope` declaration belongs to the surrounding translated function, not to a nested block containing the defer statement. Do not introduce block-local defer stacks for Go block scopes.

Named-result mutation by deferred calls and full panic/recover interaction require additional return and panic machinery and are not yet part of the frozen mapping.

## Unfrozen constructs

If no canonical translation rule exists for a Go construct, do not silently invent a project-wide convention and treat it as stable.

A translator may use a local adaptation when necessary to make progress, but it should be explicit that the mapping is provisional and may need to change when the translation contract is extended.

Translation-facing mappings with broad source impact should be frozen before large-scale porting depends on them.

Examples currently requiring additional canonical rules include:

- goroutine call evaluation and launch shape;
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
