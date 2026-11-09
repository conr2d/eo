# Parity cases

Each directory here is one behavioral parity case:

| File | Purpose |
| --- | --- |
| `main.go` | Go reference program. |
| `main.cpp` | C++ translation, written by following [the deterministic translation rules](../../docs/TRANSLATION_RULES.md). |
| `expected.stdout` | Golden stdout, generated from `main.go`. |
| `expected.exitcode` | Golden exit status, present only when it is not 0. |
| `KNOWN_GAP` | Optional. Marks the C++ translation as a known gap. The first line names the one expected mismatch (`exitcode`, `stdout`, or `timeout`). The rest says why. |
| `KNOWN_GAP.<variant>` | Same as `KNOWN_GAP`, for a variant registered with `eo_add_parity_variant`. |

## Tests

For a case `<name>`, CTest runs:

- `parity.<name>.cpp`: the C++ translation compared with the golden output.
- `parity.<name>.go`: the Go reference compared with the same golden output. It is registered only when CMake finds `go`.
- `parity.<name>.cpp.<variant>` and `parity.<name>.go.<variant>`: both sides run again under a different environment, for example `EOMAXPROCS=1` for C++ and `GOMAXPROCS=1` for Go.

Both sides are checked against the same golden files, so the C++ tests run without Go installed and the Go tests catch golden files that have gone stale.

Only stdout and the exit status are compared. Stderr is shown when a test fails but is not compared, because Go and Eo format panics differently.

A known-gap test passes only when the declared mismatch, and nothing else, occurs. It fails on a missing binary, a crash, or a different mismatch. Once the gap is fixed it fails with "known gap closed", which is the signal to delete the marker. Markers are read when the test runs, so adding or removing one does not need a CMake reconfigure.

CTest only registers a Go test when `go version` runs at configure time. The Go side sets `GOTOOLCHAIN=local`, so it never downloads a toolchain.

## Adding a case

1. Write `<name>/main.go`. Its output must be guaranteed by the Go specification and memory model: no scheduler-dependent ordering, no map iteration order, and no pseudo-random sequences.
2. Run `tests/parity/update-golden.sh <name>` to generate the golden files.
3. Write `<name>/main.cpp` by following the canonical mappings literally. For a construct that is not frozen yet, use the closest documented shape and say so in the case.
4. Add `eo_add_parity_case(<name>)` to `CMakeLists.txt`.
5. If the translation cannot match Go yet, add `<name>/KNOWN_GAP` with the mismatch kind on the first line and the reason below, and add it to the list below. Give a case that is expected to hang a short `TIMEOUT`.

The Go module in this directory pins the language version (`go 1.22`), so semantics such as per-iteration loop variables do not depend on the installed toolchain.

## Known gaps

| Case | Mismatch | Divergence |
| --- | --- | --- |
| `panic_exit` | exitcode | An uncaught panic is reported on stderr, but the process exits with status 0. Go exits with status 2. |
| `goroutine_panic` | timeout | A panic inside a goroutine is discarded by `go()`, and `main` keeps running. Go terminates the process with status 2. |
| `waitgroup_results`, variant `maxprocs1` | timeout | `sync::WaitGroup::Wait` blocks the OS thread. With one runtime worker the goroutines it waits for never run. Go completes with `GOMAXPROCS=1`. |

Panic behavior is not frozen in the translation rules yet. These cases use `throw std::runtime_error("panic: ...")`, the form the Eo runtime itself throws.
