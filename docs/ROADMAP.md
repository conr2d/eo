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
- channels, including buffering and close semantics
- `select`, including ready-case selection and default behavior
- cancellation and context propagation
- `defer`-style scope cleanup
- error and result handling patterns

Semantic compatibility matters more than making these APIs look idiomatic in C++.

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

See [Translation Principles](./TRANSLATION.md) for the identifier-preservation policy and related translation rules.

## 5. Validate against real-world Go codebases

Exercise Eo on progressively larger and more concurrency-heavy programs:

- port representative packages without redesigning their architecture
- measure how much code can be translated mechanically
- use failures to identify missing semantics or awkward translation rules
- expand conformance tests when real ports expose behavioral gaps

The validation set should span multiple application domains so that Eo does not evolve around one specific workload.

## 6. Support behavioral parity testing

Make it practical to compare translated C++ implementations with their Go references:

- run equivalent inputs through both implementations
- compare externally observable outputs and state transitions
- add differential tests for edge cases discovered during ports
- distinguish deliberate implementation differences from semantic regressions

Reference-implementation parity is a key signal that mechanical translation remains trustworthy as the project grows.

## 7. Optimize only where it is valuable

A mechanically translated implementation should be allowed to remain structurally close to Go unless there is a reason to diverge.

For performance-critical paths:

- profile before rewriting
- replace compatibility-layer code with native C++ implementations selectively
- preserve behavioral parity with the translated or reference implementation
- keep optimized code isolated enough that the original semantic mapping remains understandable

This allows Eo to serve as a fast path from Go to working C++, while still leaving room for targeted native optimization later.

## Guiding principles

- Semantic compatibility over idiomatic C++.
- Mechanical translation over redesign.
- Identifier preservation over stylistic renaming.
- Explicit differences over implicit surprises.
- Conformance tests over assumptions.
- Real-world validation over synthetic completeness.
- Domain-neutral primitives over workload-specific APIs.

The roadmap is directional rather than a release schedule. Priorities may change as larger ports expose new semantic gaps or better translation patterns.
