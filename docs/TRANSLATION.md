# Translation Principles

Eo exists to make mechanical Go-to-C++ translation predictable, reviewable, and semantically trustworthy.

The translation model should preserve source structure whenever C++ permits it. Transformations should be introduced only when they are required by C++ syntax, C++ semantics, or an explicit Eo mapping.

## Identifier preservation

When an Eo symbol corresponds directly to a Go symbol, Eo preserves the Go identifier spelling whenever practical.

For example, a Go-compatible API should prefer:

```cpp
auto timer = time::NewTimer(d);
timer->Reset(d);
timer->Stop();
auto ch = timer->C;
```

over mechanically unrelated C++ renamings such as `new_timer`, `reset`, `stop`, or `c`.

The same rule applies to mechanically ported application code. Existing Go identifiers should not be renamed merely to satisfy a C++ naming convention.

This keeps symbol mapping simple for humans and tooling, makes Go and C++ implementations easier to compare, and reduces unnecessary decisions during automated translation.

## Naming by layer

- Go-compatible Eo APIs preserve the corresponding Go names.
- Mechanically ported code preserves source Go identifiers unless C++ requires a change.
- Eo implementation details that do not correspond to Go symbols use the project's C++ naming conventions.
- Native C++ rewrites may follow the conventions of the surrounding C++ project.

Go export capitalization is preserved as spelling, but C++ visibility and linkage remain separate design concerns.

## Allowed renaming

Renaming is appropriate when:

- the Go identifier is not valid or practical C++ syntax;
- preserving the name would change semantics or create an unavoidable language conflict;
- Eo defines an explicit translation rule for the construct;
- the symbol is an Eo-specific implementation detail with no Go counterpart.

These exceptions should stay small and documented so that translation remains mechanical rather than heuristic.

## Structural fidelity

Identifier preservation is part of a broader rule: minimize source-level transformations that do not contribute to semantic correctness.

C++-specific syntax such as pointer member access, coroutine operators, ownership types, and explicit templates may still differ from Go. Those required differences should not be compounded by unrelated renaming or redesign.
