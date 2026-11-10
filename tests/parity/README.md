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
| `KNOWN_GAP.stdout`, `KNOWN_GAP.<variant>.stdout` | The exact stdout the C++ side writes, before it is killed for a `timeout` gap. Required for `stdout` and `timeout` gaps, and compared whenever present. |
| `KNOWN_GAP.exitcode`, `KNOWN_GAP.<variant>.exitcode` | The exact exit status the C++ side returns. Required for an `exitcode` gap, and allowed only for one. |

Only the Select, Defer, panic and recover, program termination, WaitGroup, and Mutex forms used here are frozen in the translation rules. Everything else in these translations is provisional, including goroutine launch, closure capture, boxed shared variables, the panic value types, string literals as `std::string`, and `map` as `std::map`. Do not copy those forms as canonical. Cases that use goroutine launch, closure capture, or boxing also say so in the first line of `main.cpp`.

## Tests

For a case `<name>`, CTest runs:

- `parity.<name>.cpp`: the C++ translation compared with the golden output.
- `parity.<name>.go`: the Go reference compared with the same golden output. It is registered only when CMake finds `go`.
- `parity.<name>.cpp.<variant>` and `parity.<name>.go.<variant>`: both sides run again under a different environment, for example `EOMAXPROCS=1` for C++ and `GOMAXPROCS=1` for Go.

Both sides are checked against the same golden files, so the C++ tests run without Go installed and the Go tests catch golden files that have gone stale.

Only stdout and the exit status are compared. Stderr is shown when a test fails but is not compared, because Go and Eo format panics differently. Programs run with no arguments and no stdin input; cases that need either are not supported yet.

A known-gap test passes only when the declared mismatch, and nothing else, occurs. It fails on a missing binary, a crash, or a different mismatch. Once the gap is fixed it fails with "known gap closed", which is the signal to delete the marker and its `.stdout` or `.exitcode` file. Markers are read when the test runs, so adding or removing one does not need a CMake reconfigure.

A gap also pins what the C++ side actually does, so a different wrong behavior does not pass for the known one. A `stdout` gap must print exactly `KNOWN_GAP.stdout`, and an `exitcode` gap must exit with exactly `KNOWN_GAP.exitcode`. A `timeout` gap must print exactly `KNOWN_GAP.stdout` before it is killed, and that output must also be a prefix of `expected.stdout`.

A crash that takes longer than `TIMEOUT` to terminate is reported as a timeout. A core dump handler such as apport can take 1 to 2 seconds per crash, so timeout gaps use `TIMEOUT 10`.

CTest only registers a Go test when `go env GOVERSION` runs at configure time and reports go1.22 or newer. The Go side sets `GOTOOLCHAIN=local`, so it never downloads a toolchain.

## Adding a case

1. Write `<name>/main.go`. Its output must be guaranteed by the Go specification and memory model: no scheduler-dependent ordering, no map iteration order, and no pseudo-random sequences.
2. Run `tests/parity/update-golden.sh <name>` to generate the golden files.
3. Write `<name>/main.cpp` by following the canonical mappings literally. For a construct that is not frozen yet, use the closest documented shape and say so in the first line of `main.cpp`.
4. Add `eo_add_parity_case(<name>)` to `CMakeLists.txt`. A directory with a `main.go` that is not registered gets a configure warning and a failing `parity.<name>.unregistered` test.
5. If the translation cannot match Go yet, add `<name>/KNOWN_GAP` with the mismatch kind on the first line and the reason below, and list it under [Known gaps](#known-gaps). Add `KNOWN_GAP.stdout` for a `stdout` or `timeout` gap and `KNOWN_GAP.exitcode` for an `exitcode` gap, and give a `timeout` case `TIMEOUT 10`.

The Go module in this directory pins the language version (`go 1.22`), so semantics such as per-iteration loop variables do not depend on the installed toolchain.

## Known gaps

There are no known gaps. List each new one here as `| Case | Mismatch | Marker |` with a link to its marker file, which gives the reason.
