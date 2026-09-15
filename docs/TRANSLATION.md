# Translation Principles

Eo exists to make mechanical Go-to-C++ translation predictable, reviewable, and semantically trustworthy.

The translation model should preserve source structure unless C++ syntax, C++ semantics, or an explicit Eo mapping requires a change. Mechanical translation should avoid unrelated restructuring so that translated code remains aligned with its Go source and upstream changes can be reapplied with minimal semantic interpretation.

When multiple C++ translations are semantically valid, prefer the form that most closely preserves the original Go identifiers, statement order, control-flow structure, and declaration boundaries.

## Compatibility target

Eo targets observable semantics defined by the Go language specification and memory model for supported, data-race-free programs.

Eo does not attempt to reproduce implementation details of a particular Go runtime version, such as exact scheduler interleavings, pseudo-random sequences, stack behavior, or timing accidents that the language does not guarantee.

This keeps the compatibility target stable as Go implementations evolve: Eo follows specified language behavior rather than cloning one runtime implementation.

## Identifier preservation

When an Eo symbol corresponds directly to a Go symbol, Eo preserves the Go identifier spelling whenever it is a valid, non-reserved C++ identifier.

For example, a Go-compatible API should prefer:

```cpp
auto timer = time::NewTimer(d);
timer->Reset(d);
timer->Stop();
auto ch = timer->C;
```

over mechanically unrelated C++ renamings such as `new_timer`, `reset`, `stop`, or `c`.

The same rule applies to mechanically ported application code. Existing Go identifiers should not be renamed merely to satisfy a C++ naming convention.

This keeps symbol mapping simple for humans and tooling, makes Go and C++ implementations easier to compare, reduces unnecessary decisions during automated translation, and makes later upstream synchronization more mechanical.

## Naming by layer

- Go-compatible Eo APIs preserve the corresponding Go names.
- Mechanically ported code preserves source Go identifiers unless C++ requires a change.
- Eo implementation details that do not correspond to Go symbols use the project's C++ naming conventions.
- Native C++ rewrites may follow the conventions of the surrounding C++ project.

Go export capitalization is preserved as spelling, but C++ visibility and linkage remain separate design concerns.

## Allowed renaming

Renaming is appropriate when:

- the Go identifier is not a valid C++ identifier;
- the spelling is reserved to the C++ implementation even though it parses as an identifier;
- preserving the name would change semantics or create an unavoidable language conflict;
- Eo defines an explicit translation rule for the construct;
- the symbol is an Eo-specific implementation detail with no Go counterpart.

These exceptions should stay small and documented so that translation remains mechanical rather than heuristic.

## Structural fidelity

Identifier preservation is part of a broader rule: minimize source-level transformations that do not contribute to semantic correctness.

Mechanically translated code should remain recognizably aligned with the source Go code. This alignment is important not only for review, but also for applying future upstream Go changes without repeatedly re-deriving the C++ implementation from scratch.

Unless C++ or an explicit Eo mapping requires otherwise, translation should preserve:

- function and method boundaries;
- source statement order;
- control-flow shape and nesting;
- declaration placement and scope;
- case, field, and declaration ordering;
- comments that remain meaningful in the translated code.

Mechanical translation should not introduce stylistic refactors such as helper extraction, condition inversion, loop restructuring, early-return rewrites, declaration hoisting, variable merging, or expression simplification merely to make the result more idiomatic C++.

Temporary variables and helper constructs may be introduced when required to preserve Go evaluation order, ownership, lifetime, coroutine behavior, or another documented semantic rule. Such transformations should be deterministic and as local as possible.

C++-specific syntax such as pointer member access, coroutine operators, ownership types, and explicit templates may still differ from Go. Those required differences should not be compounded by unrelated renaming or redesign.

Control-flow translations should preserve the source control-flow boundaries whenever possible. In particular, translation helpers should avoid introducing callbacks or nested lambdas merely to emulate source-language statements when doing so changes the meaning of `return`, `break`, or `continue`.

Native C++ optimization or redesign should be treated as a distinct step from mechanical translation so that the mechanically translated form remains suitable for source comparison and upstream synchronization.

## Select

Go `select` uses a dedicated translation rule based on an ordinary C++ `switch`, one-shot communication operations, and explicit blocking versus non-blocking selection.

See [Select Translation Design](./SELECT.md) for the canonical mapping and semantic requirements.
