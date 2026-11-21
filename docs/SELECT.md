# Select Translation Design

Status: Proposed for the translation-ready baseline.

## Purpose

Eo translates existing Go code to C++ while preserving source structure, control flow, concurrency structure, and observable behavior as closely as C++ permits.

Go `select` requires an explicit translation rule because C++ has no corresponding language construct. The translation should minimize structural changes and avoid callback boundaries, macro-heavy syntax, and hidden communication steps.

The canonical Eo representation uses a real C++ `switch`.

This is not primarily a stylistic choice. A `switch` preserves the important control-flow behavior of Go select clauses:

- an unlabeled `break` exits the select;
- a `continue` targets an enclosing loop;
- a function return remains a function return;
- variables declared by a receive remain scoped to the selected clause.

The case bodies therefore remain ordinary C++ statement blocks, while Eo-specific behavior is confined to selection and receive-result access.

## Compatibility target

Eo targets observable semantics defined by the Go language specification and memory model for supported, data-race-free programs.

Eo does not attempt to reproduce implementation details of a particular Go runtime version, including:

- exact scheduler interleavings;
- exact pseudo-random selection sequences;
- runtime stack implementation details;
- behavior that depends on C++ undefined behavior;
- implementation-specific behavior outside Go's language and memory-model guarantees.

This boundary does not weaken the Select contract. Select should implement the specified communication semantics exactly.

## Canonical translation

Communication clauses are numbered from zero in source order. `default` is not a communication clause and receives no index.

### Blocking select

Go:

```go
select {
case msg := <-messages:
    fmt.Println("received", msg)

case output <- value:
    sent()
}
```

Eo:

```cpp
switch (auto select = Select{*messages, output << value};
        co_await select.index()) {
case 0: {
  auto msg = select.recv<0>();
  fmt::Println("received"s, msg);
  break;
}
case 1: {
  sent();
  break;
}
}
```

`index()` does not merely report a ready case. When it returns, exactly one communication has already committed.

A receive value is therefore available synchronously from the selected operation. There is no second asynchronous `process<N>()` step.

### Select with default

Go:

```go
select {
case msg := <-messages:
    use(msg)

case output <- value:
    sent()

default:
    idle()
}
```

Eo:

```cpp
switch (auto select = Select{*messages, output << value};
        select.try_index()) {
case 0: {
  auto msg = select.recv<0>();
  use(msg);
  break;
}
case 1: {
  sent();
  break;
}
default: {
  idle();
  break;
}
}
```

A Go select containing `default` cannot block, so its Eo translation contains no `co_await`.

`try_index()` either commits one ready communication and returns its index or returns `-1` immediately.

The canonical distinction is:

```text
select without default  -> co_await select.index()
select with default     -> select.try_index()
```

A non-blocking Go select therefore does not force an otherwise synchronous C++ function to become a coroutine.

## Receive forms

### Receive and discard

Go:

```go
case <-ch:
    signal()
```

Eo:

```cpp
case N: {
  signal();
  break;
}
```

The communication has already completed when the body begins, so no accessor is required.

### Receive with declaration

Go:

```go
case x := <-ch:
    use(x)
```

Eo:

```cpp
case N: {
  auto x = select.recv<N>();
  use(x);
  break;
}
```

### Two-value receive

Go:

```go
case x, ok := <-ch:
    if !ok {
        return
    }
    use(x)
```

Eo:

```cpp
case N: {
  auto [x, ok] = select.recv2<N>();
  if (!ok) {
    co_return;
  }
  use(x);
  break;
}
```

`recv2<N>()` is part of the intended Select architecture even if its implementation lands separately.

For a receive selected because a channel is closed and drained:

```text
recv<N>()  -> zero value
recv2<N>() -> {zero value, false}
```

For a successfully received value:

```text
recv2<N>() -> {value, true}
```

### Receive assignment

Go evaluates receive-assignment left-hand-side expressions only after that communication has been selected.

For example:

```go
case values[index()] = <-ch:
    use(values)
```

must translate so that `index()` is not evaluated while entering the select.

The canonical Eo form naturally preserves this:

```cpp
case N: {
  values[index()] = select.recv<N>();
  use(values);
  break;
}
```

The channel operand belongs in `Select{...}`. The assignment target remains inside the selected case body.

## Operand evaluation

Go evaluates, exactly once and in source order upon entering a select:

- every receive channel operand;
- every send channel operand;
- every send right-hand-side expression.

The canonical translation therefore requires a braced initializer:

```cpp
Select{op0, op1, op2}
```

Case operations appear in the same order as their source Go communication clauses.

For example:

```go
select {
case x := <-getChannel():
case output() <- makeValue():
}
```

translates to:

```cpp
auto _eo_call_0 = getChannel();
auto _eo_call_1 = output();
auto _eo_call_2 = makeValue();
switch (auto select = Select{*_eo_call_0, _eo_call_1 << _eo_call_2};
        co_await select.index()) {
  // ...
}
```

Each expression is evaluated once when the `Select` object is constructed. The case operands are one full expression for [Evaluation order](./TRANSLATION_RULES.md#evaluation-order), so calls among them are saved in `_eo_call_<n>` temporaries before the `switch` statement, in source order. A select whose operands need no temporaries by those rules keeps them in `Select{...}`.

The translation must not hoist a `Select` outside a loop, because Go reevaluates the case operands every time execution reaches the select statement.

## One-shot lifetime

A `Select` represents exactly one execution of exactly one source Go select.

The canonical loop form is:

```cpp
for (;;) {
  switch (auto select = Select{*a, *b};
          co_await select.index()) {
  case 0: {
    // ...
    break;
  }
  case 1: {
    // ...
    break;
  }
  }
}
```

This is intentionally not:

```cpp
auto select = Select{*a, *b};

for (;;) {
  switch (co_await select.index()) {
    // ...
  }
}
```

The latter changes operand evaluation semantics and may incorrectly reuse a send value or channel expression.

`Select` should therefore be non-copyable, non-movable, and executable only once.

## Case body rule

Every translated communication clause uses a braced C++ case body:

```cpp
case N: {
  // ...
  break;
}
```

The rule is unconditional.

This preserves Go's clause-local variable scope and avoids position-dependent C++ initialization restrictions.

Each case whose body does not end with a terminating statement, as the Go specification defines it in "Terminating statements", ends with an explicit `break`. A `return`, a `goto`, and a `panic` call are terminating statements. A `continue`, a `break L`, and a `continue L` are not, so a body that ends with one of them still ends with `break;`. A body that ends with an unlabeled `break` already ends with it.

## Control flow

The Select mapping itself does not translate function return syntax. Return translation belongs to the surrounding function mapping.

Within a translated select:

- an unlabeled `break` remains `break`;
- a `continue` remains `continue`;
- a return follows the normal translation rule for that function;
- a labeled `break` or `continue`, and a `goto`, follow [Labeled statements](./TRANSLATION_RULES.md#labeled-statements): each becomes a C++ `goto` to the label that rule places, which is outside the `switch` when the target is a statement outside the clause, and inside the clause's block when the target is a statement in it.

Using a real C++ `switch` is important because it preserves the targets of ordinary `break` and `continue` without introducing callback boundaries.

## Public API

The intended public shape is:

```cpp
namespace eo {

template<typename... Cases>
class Select {
public:
  explicit Select(Cases... cases);

  Select(const Select&) = delete;
  Select(Select&&) = delete;
  auto operator=(const Select&) -> Select& = delete;
  auto operator=(Select&&) -> Select& = delete;

  auto index() -> func<int>;
  auto try_index() -> int;

  template<size_t N>
  auto recv();

  template<size_t N>
  auto recv2();
};

template<typename... Cases>
Select(Cases...) -> Select<Cases...>;

} // namespace eo
```

No public `CaseDefault` operation is required.

The default clause is represented by the choice of `try_index()` and the ordinary C++ `default:` label.

`recv<N>()` and `recv2<N>()` are valid only for receive cases.

The implementation should reject or diagnose:

- an out-of-range accessor index;
- a receive accessor used for a send case;
- an accessor for a case that was not selected;
- consuming the same receive result twice;
- calling `index()` or `try_index()` more than once.

Errors detectable from case types or indices should be compile-time errors. State-dependent misuse should fail immediately at runtime rather than silently performing another communication.

## Empty select

Go:

```go
select {}
```

blocks forever.

Eo should support the equivalent blocking Select operation with no possible communication. The exact zero-case construction form may be chosen during implementation, but it must preserve this behavior without introducing a special scheduler dependency into the translation rule.

## Nil channels

Nil channel behavior is part of the Select semantic contract.

In Go:

- receive from a nil channel blocks forever;
- send to a nil channel blocks forever;
- a nil channel case in a select can never proceed;
- a select containing only nil communication cases and no default blocks forever;
- a select containing only nil communication cases and a default selects default immediately;
- closing a nil channel panics.

Eo's channel representation must therefore be capable of representing a nil channel independently of an allocated channel.

Select must treat nil communication operations as permanently disabled rather than as errors.

This requirement is independent of the surface Select syntax and should be satisfied before declaring channel/select semantics conformant.

## Closed channels

A receive from a closed and drained channel is immediately ready.

If selected:

```text
recv<N>()  -> zero value
recv2<N>() -> {zero value, false}
```

A send to a closed channel is also a selectable operation whose execution panics.

When a closed send and another ready operation coexist, normal ready-case selection semantics still apply. The implementation must not panic merely because a closed-send case exists. It panics if that send case is selected.

## Ready-case selection

If multiple communications can proceed, Select chooses one using a uniform pseudo-random selection strategy.

The implementation does not need to reproduce the exact random sequence of any Go runtime.

It does need to avoid deterministic source-order bias and ensure that exactly one communication commits.

A randomized probe order is sufficient for an immediate non-blocking selection if probing and commitment are atomic with respect to the underlying channel operation.

## Exactly-one arbitration

The public Select contract is:

> When `index()` or a successful `try_index()` returns, exactly one communication has committed.

The case body must never be responsible for completing that communication.

For the non-blocking path, Select may synchronously probe candidate operations in randomized order and stop after the first committed communication.

For the blocking path, cancellation of losing asynchronous operations is not by itself a sufficient correctness mechanism. Multiple channel operations may become ready before cancellation reaches the losers.

Eo therefore owns its channel core, in `<eo/chan.h>`, modeled on Go's `hchan` and `selectgo`:

- Each channel has one mutex, a buffer, and queues of blocked receivers and blocked senders.
- A blocked goroutine waits on one `wait_token`, which holds its resumption handler and an atomic `done` flag. A single send or receive has its own token. A blocking select has one token, which every one of its waiters refers to.
- An operation that finds a queued waiter, a send finding a receiver or a receive finding a sender, and `close`, first claims the waiter's token with a compare-and-swap on `done`. Only the operation that claims the token completes the communication: it moves the value, records the case index, and posts the handler to the waiting goroutine's executor. A queued waiter whose token is already claimed belongs to a select that chose another case, and the operation skips it.
- A blocking select first probes its cases in a random order, as `try_index()` does. If none is ready, it locks the channels of all its cases in address order, so that two selects cannot deadlock, probes them again in a random order, and otherwise queues one waiter per case under its token and unlocks the channels. When it resumes, it removes its other waiters from their queues before it returns the index.
- A waiting goroutine can resume on another thread as soon as its handler is posted. So an operation that completes another goroutine's token touches its waiter only before it posts the handler, and a select or single operation that finds a case ready while it arms its own wait releases its locks first and completes its token as its last action. A resumed select removes its waiters under each channel's lock, so it cannot pass an operation that still holds a pointer to one of them.

- The code between claiming a token and completing it, and a select's code between locking its channels and unlocking them, is `noexcept`. A throw there could only come from running out of memory, or from a throwing move of a value, which no translated type has. Like Go's runtime, Eo treats running out of memory as fatal, so such a throw terminates the program instead of leaving a goroutine blocked and a channel locked.

So a losing case never receives or sends a value: its waiter can complete only by claiming the token, which the winning case already holds. `tests/select_exactly_one.cpp` checks this on a multi-threaded pool, for losing receives and for losing sends.

The syntax described in this document does not depend on this arbitration mechanism.

## Receive result storage

A selected receive stores its result in the operation object owned by the `Select`.

The result remains valid for the lifetime of the switch statement.

`recv<N>()` or `recv2<N>()` extracts the already committed result into the case body.

An unselected receive has no observable value.

This avoids placeholder values for losing cases and makes a mismatched case/accessor detectable.

## Zero values

Go types always have a zero value.

Closed receive semantics therefore require the translated representation of a Go type to have a corresponding Eo zero-value operation.

For simple C++ representations this may be `T{}`. Eo should not assume that C++ default construction is sufficient for every future translated Go type; zero-value construction is a broader translation concern and should have an explicit rule.

## Interaction with defer

Select case bodies must preserve Go lexical scopes even if Eo's current defer implementation does not.

The Select design therefore always uses braced case bodies and does not alter its scopes to accommodate `eo_defer`.

A Go defer is function-scoped, not block-scoped. For example:

```go
for {
    select {
    case x := <-ch:
        defer cleanup(x)
        // ...
    }
}
```

does not run `cleanup` when the case body ends. Each executed defer is registered for the eventual exit of the surrounding function.

A lexical RAII scope guard is therefore not a sufficient general implementation of Go defer.

Eo should move toward a function-scoped defer stack. A defer registration must preserve Go's rules:

1. the deferred function value and direct-call arguments are evaluated when the defer statement executes;
2. the call itself executes when the surrounding function returns or unwinds because of an Eo-modeled Go panic;
3. deferred calls execute in LIFO order;
4. executing the same defer statement repeatedly, including inside a loop, registers a new deferred call each time.

Full Go defer parity additionally requires integration with return handling for named result variables. That is a separate translation-runtime concern and must not be hidden inside Select.

## Panic and recover boundary

A complete Go defer implementation is coupled to Go panic/recover semantics.

Eo may model Go panics through an Eo-controlled runtime mechanism, but native C++ undefined behavior cannot be turned into a recoverable Go panic, and arbitrary native C++ exceptions are not automatically Go panics.

The compatibility contract should distinguish Eo-modeled Go behavior from failures originating outside the compatibility model.

## Nearby semantics to audit before large-scale porting

Freezing Select syntax is necessary before a large port, but several nearby rules also need explicit conformance checks. Each of these is now covered by a rule and a parity case (see [Translation-ready Baseline](./BASELINE.md)).

### Goroutine launch evaluation

Go evaluates the function value and arguments of a `go` statement in the launching goroutine before the new goroutine begins executing.

Translation through reference-capturing lambdas must not accidentally defer those evaluations until the scheduled callable runs.

### Labeled control flow

Go supports labeled `break` and `continue`.

[Labeled statements](./TRANSLATION_RULES.md#labeled-statements) maps them, and `goto`, to C++ `goto` statements. A `break L` that targets a select jumps to `_eo_break_L:;` after its `switch` statement. Select's real `switch` representation allows these jumps because it does not introduce a callback boundary.

### Nil and zero values

Go relies pervasively on meaningful zero values, including nil channels, pointers, interfaces, slices, and maps.

Each Eo representation should state its zero-value mapping instead of relying implicitly on C++ default construction.

### Panic-producing operations

Go defines panics for operations that may instead become exceptions, library errors, or undefined behavior in naive C++ translations.

Where behavioral parity matters, mechanically translated code should use Eo operations that preserve the relevant Go failure semantics.

## Hard compatibility boundaries

Not every Go program can have identical semantics under ordinary C++ execution.

The most important hard boundary is data races. Ordinary C++ data races can invoke undefined behavior, so Eo does not promise Go-compatible behavior for source programs that rely on data races.

Other implementation-level differences may remain observable in programs that depend on behavior Go does not guarantee, including exact scheduling, exact pseudo-random choices, runtime stack behavior, or timing accidents.

The purpose of Eo conformance is to preserve Go's specified program behavior, not reproduce every implementation artifact of the Go runtime.

## Migration from the current Select API

Existing code of the form:

```cpp
auto select = Select{*a, *b, CaseDefault{}};

switch (co_await select.index()) {
case 0: {
  auto x = co_await select.process<0>();
  // ...
  break;
}
// ...
}
```

migrates mechanically.

For each source select:

1. move Select construction into the `switch` init-statement;
2. construct it at every execution of the original Go select;
3. remove `CaseDefault`;
4. use `try_index()` when the source select contains default;
5. otherwise use `co_await index()`;
6. replace receive `process<N>()` with synchronous `recv<N>()`;
7. remove `process<N>()` for send cases;
8. use `recv2<N>()` for two-value receives;
9. brace every case body;
10. keep communication indices in source communication-clause order.

The old `process<N>()` and `CaseDefault` APIs should be removed rather than retained as compatibility aliases once in-repository call sites have migrated.

## Implementation sequence

The surface syntax should be separated from deeper runtime work so each correctness change remains reviewable.

### Phase 1 — freeze Select syntax

Implement:

- switch-init canonical form;
- one-shot Select enforcement;
- `index()`;
- `try_index()`;
- synchronous `recv<N>()`;
- receive-result state needed for future `recv2<N>()`;
- removal of `CaseDefault`;
- removal of public `process<N>()`;
- migration of examples, tests, and documentation.

This phase must not claim that the existing blocking arbitration is fully conformant if the underlying channel implementation can still commit multiple waiters.

### Phase 2 — channel semantic prerequisites

Implement or verify:

- nil channel representation;
- closed receive value/`ok` state;
- closed send selection and panic timing;
- zero-value behavior;
- non-blocking probing semantics;
- two-value receive plumbing.

### Phase 3 — exactly-one blocking arbitration

Introduce a channel core or equivalent mechanism that allows selection arbitration before a communication commits.

Add conformance and stress tests demonstrating that losing operations never consume, send, or otherwise mutate channel state.

Done: see [Exactly-one arbitration](#exactly-one-arbitration).

### Phase 4 — defer semantics

Replace lexical-scope defer behavior with function-scoped registration.

Cover:

- execution from nested blocks and select clauses;
- registration in loops;
- immediate argument evaluation;
- LIFO order;
- early function return;
- coroutine return;
- named result interaction.

Panic/recover integration may be staged separately if required, but the supported subset must be documented explicitly.

## Decision

Freeze the Select translation shape as:

```text
blocking:
  switch (auto select = Select{operations...};
          co_await select.index())

with default:
  switch (auto select = Select{operations...};
          select.try_index())
```

with:

```text
receive binding       -> select.recv<N>()
two-value receive     -> select.recv2<N>()
receive discard       -> no accessor
send                  -> no accessor
default               -> ordinary C++ default:
case body             -> always braced
Select lifetime       -> one source-select execution
communication commit  -> before the selected case body begins
```

This design intentionally favors mechanical translation, visible suspension points, preserved control flow, and explicit failure modes over idiomatic C++ abstraction.

The syntax is independent of the eventual channel arbitration implementation and should remain stable while the runtime is strengthened underneath it.
