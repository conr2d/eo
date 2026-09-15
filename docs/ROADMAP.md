# Roadmap

Eo is a C++ compatibility layer for mechanically porting existing Go code while preserving control flow, concurrency structure, and observable behavior as closely as practical.

The roadmap focuses on making that translation model predictable, testable, and useful across real-world Go codebases. It intentionally avoids tying Eo to a particular industry or application domain.

## 1. Stabilize the C++23 foundation

- keep the project on a clear C++23 baseline
- keep dependency discovery and build configuration simple and reproducible
- keep runtime startup and common failure modes predictable
- maintain CI coverage for supported toolchains and formatting rules

## 2. Match core Go semantics

Prioritize language and runtime behavior that strongly affects mechanical translation:

- goroutine-style execution
- channels, including buffering, nil, and close semantics
- `select`, including ready-case selection, default behavior, and exactly-one communication
- cancellation and context propagation
- function-scoped `defer` semantics
- error and result handling patterns

Semantic compatibility matters more than making these APIs look idiomatic in C++.

Eo targets observable behavior defined by the Go language specification and memory model for supported, data-race-free programs. It does not attempt to reproduce implementation details of a particular Go runtime version.

## 3. Harden public translation primitives

Remove contracts that make direct translation fragile or surprising:

- avoid hidden lifetime requirements
- make value-like handles behave consistently when copied or moved
- make cancellation, runtime, and registration APIs safe under expected concurrent use
- propagate failures instead of silently changing program behavior
- keep translation-facing APIs small and mechanically recognizable
- preserve Go identifier spelling for translation-facing APIs whenever C++ permits it

APIs remain unstable while this layer is being refined.

## 4. Define repeatable Go-to-C++ translation rules

Build a translation model that can be applied consistently by humans and tooling:

- document common syntax and control-flow mappings
- identify patterns that can be translated mechanically
- identify cases that require semantic adaptation
- preserve source identifiers unless C++ syntax, semantics, or an explicit Eo mapping requires a change
- minimize transformations that obscure the structure of the source Go code
- keep unsupported or intentionally different behavior explicit

The goal is not source compatibility. The goal is a small, stable set of rules that makes structural translation fast and reviewable.

See [Translation Principles](./TRANSLATION.md) for compatibility and structural-fidelity policy, [Deterministic Translation Rules](./TRANSLATION_RULES.md) for the mechanical translation contract, and [Select Translation Design](./SELECT.md) for the canonical `select` mapping.

## Translation-ready baseline

Before beginning a large real-world port, translation-facing syntax and the most disruptive semantic mappings should be stable enough that the port does not need repeated repository-wide rewrites.

The current path to that baseline is:

1. **Freeze Select translation syntax.** Adopt one-shot `Select`, `index()` for blocking selects, `try_index()` for selects with `default`, synchronous receive accessors, and switch-scoped lifetime.
2. **Close channel/select edge-semantic gaps.** Cover nil channels, closed receive status, closed-send panic timing, two-value receive plumbing, and explicit zero-value behavior.
3. **Guarantee exactly-one blocking Select communication.** Strengthen the underlying channel/select arbitration without tying the public syntax to one runtime algorithm.
4. **Fix `defer` semantics.** Replace lexical-scope cleanup with function-scoped registration and preserve evaluation timing, LIFO execution, loops, early return, and coroutine return. Named-result interaction may require explicit return machinery.
5. **Audit adjacent translation rules.** Verify goroutine launch evaluation, labeled control flow, zero values, and other panic-producing operations that can silently diverge under naive C++ translation.
6. **Declare the translation-ready baseline.** Document the supported semantic surface and known boundaries, then begin real-world porting and evolve Eo demand-first from concrete failures.

This is a readiness milestone, not a claim of Go runtime completeness. The goal is to stabilize translation rules and specification-level behavior that would otherwise be expensive to change after a large port begins.

## 5. Validate against real-world Go codebases

Exercise Eo on progressively larger and more concurrency-heavy programs:

- port representative packages without redesigning their architecture
- measure how much code can be translated mechanically
- use failures to identify missing semantics or awkward translation rules
- expand conformance tests when real ports expose behavioral gaps

The validation set should span multiple application domains so that Eo does not evolve around one specific workload.

After the translation-ready baseline, real ports should become a primary source of new Eo work rather than continuing to expand the compatibility layer speculatively.

## 6. Support behavioral parity testing

Make it practical to compare translated C++ implementations with their Go references:

- run equivalent inputs through both implementations
- compare externally observable outputs and state transitions
- add differential tests for edge cases discovered during ports
- distinguish deliberate implementation differences from semantic regressions

Reference-implementation parity is a useful validation signal, but the compatibility contract remains the behavior guaranteed by the Go language specification and memory model rather than implementation details of one Go runtime.

## 7. Optimize only where it is valuable

A mechanically translated implementation should be allowed to remain structurally close to Go unless there is a reason to diverge.

For performance-critical paths:

- profile before rewriting
- replace compatibility-layer code with native C++ implementations selectively
- preserve behavioral parity with the translated or reference implementation
- keep optimized code isolated enough that the original semantic mapping remains understandable

This allows Eo to serve as a fast path from Go to working C++, while still leaving room for targeted native optimization later.

## Guiding principles

- Specification-level semantic compatibility over runtime implementation cloning.
- Semantic compatibility over idiomatic C++.
- Mechanical translation over redesign.
- Identifier preservation over stylistic renaming.
- Explicit differences over implicit surprises.
- Conformance tests over assumptions.
- Real-world validation over synthetic completeness.
- Domain-neutral primitives over workload-specific APIs.

The roadmap is directional rather than a release schedule. Priorities may change as larger ports expose new semantic gaps or better translation patterns.
