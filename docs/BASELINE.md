# Translation-ready Baseline

This document declares the translation-ready baseline that the [Roadmap](./ROADMAP.md#translation-ready-baseline) set as the condition for beginning a large real-world port. It records what a translation may rely on, where the known boundaries are, and how Eo changes from here.

The baseline is a readiness milestone, not a claim of Go runtime completeness. It means that the translation-facing syntax and the most disruptive semantic mappings are stable enough that a port should not need repeated repository-wide rewrites. Eo APIs are still unstable in general. The stability described here applies to the frozen translation forms.

## What the baseline covers

A translation may rely on the forms that the translation rules freeze, and on the behavior that the parity cases check.

- [Deterministic Translation Rules](./TRANSLATION_RULES.md#canonical-construct-mappings) is the mechanical contract. Only the mappings it freezes are canonical.
- [Select Translation Design](./SELECT.md) freezes the `select` shape and its semantics.
- [Goroutine Translation Design](./GOROUTINE.md) freezes the `go` statement shape and the launch evaluation contract.
- [Translation Principles](./TRANSLATION.md) sets the identifier-preservation and structural-fidelity policy that every rule follows.

Each area below has a canonical rule and at least one [parity case](../tests/parity/README.md), which runs a Go program and its canonical translation against the same golden output. Cases whose result could depend on scheduling also run with one worker thread, as `EOMAXPROCS=1` for C++ and `GOMAXPROCS=1` for Go.

| Area | Rules | Parity cases include |
|---|---|---|
| Basic types, arithmetic, conversions, constants | [Basic types](./TRANSLATION_RULES.md#basic-types) | `integer_arithmetic`, `integer_panic_recover`, `constants` |
| Evaluation order | [Evaluation order](./TRANSLATION_RULES.md#evaluation-order) | `eval_order_calls`, `eval_order_receives` |
| Identifiers | [Identifier mapping](./TRANSLATION_RULES.md#identifier-mapping) | `redeclared_names`, `struct_names` |
| Strings | [Strings](./TRANSLATION_RULES.md#strings) | `string_bytes`, `string_index_panic`, `string_index_recover` |
| Slices | [Slices](./TRANSLATION_RULES.md#slices) | `slice_alias`, `slice_element_address`, `slice_index_recover` |
| Arrays | [Arrays](./TRANSLATION_RULES.md#arrays) | `array_values`, `array_slicing`, `array_defined` |
| The `slices` package, in part | [The slices package](./TRANSLATION_RULES.md#the-slices-package) | `slices_package` |
| Maps and the `maps` package | [Maps](./TRANSLATION_RULES.md#maps) | `map_values`, `map_nil_panic`, `map_range_mutation`, `maps_package` |
| Structs and embedding | [Structs](./TRANSLATION_RULES.md#structs) | `struct_copy`, `struct_literal`, `struct_embedding` |
| Defined types | [Defined types](./TRANSLATION_RULES.md#defined-types) | `defined_types` |
| Switch statements | [Switch statements](./TRANSLATION_RULES.md#switch-statements) | `switch_values`, `switch_case_order`, `switch_jumps` |
| Range loops | [Range loops](./TRANSLATION_RULES.md#range-loops) | `range_loop`, `range_variable_address`, `channel_range` |
| Labeled statements and `goto` | [Labeled statements](./TRANSLATION_RULES.md#labeled-statements) | `labels` |
| Pointers and escaping variables | [Pointers and escaping variables](./TRANSLATION_RULES.md#pointers-and-escaping-variables) | `escape_return_pointer`, `field_address`, `closure_capture_by_ref`, `loop_variable_address`, `unsafe_pointers`, `unsafe_slices` |
| Function values and methods | [Function values](./TRANSLATION_RULES.md#function-values) | `func_values`, `func_coroutines`, `value_receiver_escape` |
| Interfaces, type assertions, type switches | [Interfaces](./TRANSLATION_RULES.md#interfaces) | `interface_values`, `interface_printing`, `type_assertion` |
| Multiple results | [Multiple results](./TRANSLATION_RULES.md#multiple-results) | `multi_value_results`, `multi_value_defer` |
| Errors | [Errors](./TRANSLATION_RULES.md#errors) | `error_values` |
| Printing and `fmt` verbs | [Formatted printing](./TRANSLATION_RULES.md#formatted-printing) | `println_operands`, `format_verbs`, `map_println` |
| The `math/big` package, in part | [The math/big package](./TRANSLATION_RULES.md#the-mathbig-package) | `big_int`, `big_int_random` |
| The `strings`, `bytes`, `unicode`, and `math/bits` packages, in part | [The strings and bytes packages](./TRANSLATION_RULES.md#the-strings-and-bytes-packages), [The unicode package](./TRANSLATION_RULES.md#the-unicode-package), [The math/bits package](./TRANSLATION_RULES.md#the-mathbits-package) | `strings_package`, `strings_bytes_random`, `unicode_case`, `bits_package` |
| The `strconv` package, in part | [The strconv package](./TRANSLATION_RULES.md#the-strconv-package) | `strconv_package` |
| The `encoding/hex` and `encoding/binary` packages, in part | [The encoding/hex package](./TRANSLATION_RULES.md#the-encodinghex-package), [The encoding/binary package](./TRANSLATION_RULES.md#the-encodingbinary-package) | `hex_package`, `binary_package` |
| The `hash` and `crypto/subtle` packages | [The hash package](./TRANSLATION_RULES.md#the-hash-package), [The crypto/subtle package](./TRANSLATION_RULES.md#the-cryptosubtle-package) | `subtle_package` |
| The `database/sql/driver` package, in part | [The database/sql/driver package](./TRANSLATION_RULES.md#the-databasesqldriver-package) | `sql_driver` |
| The `encoding`, `encoding/base64`, and `unicode/utf16` packages, and `slices.Grow` | [The encoding package](./TRANSLATION_RULES.md#the-encoding-package), [The encoding/base64 package](./TRANSLATION_RULES.md#the-encodingbase64-package), [The unicode/utf16 package](./TRANSLATION_RULES.md#the-unicodeutf16-package), [The slices package](./TRANSLATION_RULES.md#the-slices-package) | `base64_package` |
| The `math` and `cmp` packages, `strconv.ParseFloat`, the `unicode` category predicates, and `slices.Equal`, `EqualFunc`, `Compare`, and `CompareFunc` | [The math package](./TRANSLATION_RULES.md#the-math-package), [The cmp package](./TRANSLATION_RULES.md#the-cmp-package), [The strconv package](./TRANSLATION_RULES.md#the-strconv-package), [The unicode package](./TRANSLATION_RULES.md#the-unicode-package), [The slices package](./TRANSLATION_RULES.md#the-slices-package) | `math_package` |
| Interface and channel map keys, and `sync.Map`, `sync.Pool`, `sync.Once`, `sync.OnceFunc`, `sync.OnceValue`, and `sync.OnceValues` | [Maps](./TRANSLATION_RULES.md#maps), [Once](./TRANSLATION_RULES.md#once), [Map and Pool](./TRANSLATION_RULES.md#map-and-pool) | `sync_package` |
| The `encoding/json` package | [The encoding/json package](./TRANSLATION_RULES.md#the-encodingjson-package) | `json_package` |
| The `io` package, `bytes.Buffer`, `bytes.Reader`, and `strings.Reader`, in part | [The io package](./TRANSLATION_RULES.md#the-io-package), [Buffers and readers](./TRANSLATION_RULES.md#buffers-and-readers) | `io_package`, `buffer_readers` |
| The `bufio` and `unicode/utf8` packages | [The bufio package](./TRANSLATION_RULES.md#the-bufio-package), [The unicode/utf8 package](./TRANSLATION_RULES.md#the-unicodeutf8-package) | `bufio_package` |
| The `reflect` package and struct field tables | [The reflect package](./TRANSLATION_RULES.md#the-reflect-package), [Field tags and embedding](./TRANSLATION_RULES.md#field-tags-and-embedding) | `reflect_package` |
| `fmt`'s flags, widths, precisions, operand indexes, `%+v`, `%#v`, and `%p`, and `strconv.FormatFloat` | [Formatted printing](./TRANSLATION_RULES.md#formatted-printing), [The strconv package](./TRANSLATION_RULES.md#the-strconv-package) | `fmt_flags`, `fmt_random` |
| `fmt.Formatter` and `fmt.State`, the `Print`, `Fprint`, and `Append` families over an `io.Writer`, methods of unexported fields, and `(*big.Int).Format` with flags | [Printing methods](./TRANSLATION_RULES.md#printing-methods), [Formatted printing](./TRANSLATION_RULES.md#formatted-printing), [The math/big package](./TRANSLATION_RULES.md#the-mathbig-package) | `fmt_formatter` |
| Goroutines | [Goroutine](./TRANSLATION_RULES.md#goroutine) | `shared_variable`, `method_goroutine`, `loop_variable_goroutine` |
| Channels | [Channels](./TRANSLATION_RULES.md#channels) | `channel_buffered`, `channel_panics`, `channel_recv_ok` |
| `select` | [Select](./TRANSLATION_RULES.md#select) | `select_default`, `labels` |
| `defer` | [Defer](./TRANSLATION_RULES.md#defer) | `defer_args`, `defer_in_loop`, `loop_variable_defer` |
| Panic and recover | [Panic and recover](./TRANSLATION_RULES.md#panic-and-recover) | `panic_runs_defers`, `recover_values`, `recover_named_result`, `recover_runtime_error`, `repanic_in_defer` |
| `sync.WaitGroup`, `sync.Mutex`, `sync.RWMutex` | [Blocking operations](./TRANSLATION_RULES.md#blocking-operations) | `waitgroup_results`, `mutex_counter`, `rwmutex_readers` |
| Package-level variables and `init` | [Package-level variables](./TRANSLATION_RULES.md#package-level-variables) | `package_vars`, `init_panic` |
| Packages of several files, and programs of several packages | [Packages](./TRANSLATION_RULES.md#packages) | `package_files`, `package_imports` |
| Program termination | [Program termination](./TRANSLATION_RULES.md#program-termination) | `main_returns_early`, `exit_from_goroutine`, `goroutine_panic` |

The runtime guarantees behind these rules are covered by the unit tests in [`tests`](../tests), including the ones that need concurrency to observe:

- a blocking `select` commits exactly one communication, and a losing case never sends or receives a value (see [Exactly-one arbitration](./SELECT.md#exactly-one-arbitration));
- nil channels, closed channels, closed-send panics, and two-value receives behave as Go specifies (see [Nil channels](./SELECT.md#nil-channels) and [Closed channels](./SELECT.md#closed-channels));
- `defer` is function-scoped, evaluates its arguments when the `defer` statement executes, runs in LIFO order on every return path, including coroutine return, and interacts with named results as Go does (see [Defer](./TRANSLATION_RULES.md#defer)).

CI runs every test, including the Go side of every parity case, with the default GCC of Ubuntu 24.04. It runs the C++ tests again under AddressSanitizer with UndefinedBehaviorSanitizer and under ThreadSanitizer, each with the default worker count and with `EOMAXPROCS=1`.

## Known boundaries

A port will reach these boundaries. Each one is either a deliberate difference or a gap that is tracked.

### Hard boundaries

These follow from executing translated code as ordinary C++, and are not expected to change.

- **Data races.** A C++ data race is undefined behavior, so Eo makes no promise for a source program that relies on one (see [Hard compatibility boundaries](./SELECT.md#hard-compatibility-boundaries)).
- **Runtime implementation details.** Eo matches what the Go specification and memory model guarantee, not exact scheduling, pseudo-random choices, timing, stack growth, or map iteration order.
- **Garbage collection.** A pointer is reference-counted, so a reference cycle is never freed (see [Pointers](./TRANSLATION_RULES.md#pointers-and-escaping-variables)). How often a real port creates cycles is tracked in [#80](https://github.com/conr2d/eo/issues/80).
- **Panic output.** A panic that ends the program exits with Go's status, but its message on stderr is not formatted as Go formats it.
- **Platform.** Eo targets 64-bit platforms with IEEE 754 `float` and `double` (see [Basic types](./TRANSLATION_RULES.md#basic-types)).

### Constructs without a canonical rule

[Unsupported constructs](./TRANSLATION_RULES.md#unsupported-constructs) lists what cannot be translated under the current contract, such as `sync.Once`, `sync.Cond`, and `defer` of a blocking call. [Unfrozen constructs](./TRANSLATION_RULES.md#unfrozen-constructs) lists what has no frozen rule yet, such as arrays, complex numbers, `a, b = b, a`, and iterator functions. Most rule sections also end with the forms they do not cover yet.

A translation that needs one of these forms writes it as an explicit, local adaptation and marks it as provisional, as [Unfrozen constructs](./TRANSLATION_RULES.md#unfrozen-constructs) requires. It does not approximate the Go behavior silently.

### Open issues

The known gaps that are tracked as issues, grouped by the kind of work they need:

- **Program structure.** Packages with several files and programs with several packages ([#123](https://github.com/conr2d/eo/issues/123)). A real port needed this before anything else, because a Go package is almost never one file. [Packages](./TRANSLATION_RULES.md#packages) now covers it, except for the forms in [Packages not covered yet](./TRANSLATION_RULES.md#packages-not-covered-yet).
- **Translation forms.** Assignments and declarations of several values that are not one call, such as `a, b = b, a` ([#111](https://github.com/conr2d/eo/issues/111)), range over functions ([#98](https://github.com/conr2d/eo/issues/98)), variadic calls in `go` and `defer` ([#95](https://github.com/conr2d/eo/issues/95)), converting a slice to a pointer to an array ([#131](https://github.com/conr2d/eo/issues/131)), Eo names that collide with Go identifiers ([#83](https://github.com/conr2d/eo/issues/83)), and the remaining `go` and `defer` launch forms ([#67](https://github.com/conr2d/eo/issues/67)).
- **Interfaces.** Methods translated as coroutines ([#108](https://github.com/conr2d/eo/issues/108)), and Go `int` and `int64` sharing a dynamic type ([#107](https://github.com/conr2d/eo/issues/107)).
- **Libraries.** The rest of the `slices` package ([#94](https://github.com/conr2d/eo/issues/94)), and `sync.Once` and `sync.Cond` ([#76](https://github.com/conr2d/eo/issues/76)).
- **Runtime.** A goroutine blocked in a system call occupies its worker thread ([#71](https://github.com/conr2d/eo/issues/71)), and a goroutine started by `WithCancel` can outlive a borrowed custom parent ([#57](https://github.com/conr2d/eo/issues/57)).
- **Pointers.** The nested-block boxing rule boxes more variables than it needs to ([#81](https://github.com/conr2d/eo/issues/81)), and reference cycles in a real port are not measured yet ([#80](https://github.com/conr2d/eo/issues/80)).
- **Testing.** Checking translations against Go's escape facts ([#79](https://github.com/conr2d/eo/issues/79)), parity harness follow-ups ([#65](https://github.com/conr2d/eo/issues/65)), and CI with fmt 11 or later ([#63](https://github.com/conr2d/eo/issues/63)).

The [open issues](https://github.com/conr2d/eo/issues) are the current list. This section is a snapshot taken when the baseline was declared.

## After the baseline

From the baseline on, Eo evolves demand-first: real ports, not speculative completeness, decide what Eo covers next (see [Validate against real-world Go codebases](./ROADMAP.md#5-validate-against-real-world-go-codebases)).

When a port reaches a construct that the rules do not freeze:

1. Write a provisional local adaptation, marked as provisional, so the port can continue.
2. Open an issue that names the construct and the source that needs it, or add the source to an existing issue.
3. When the construct is frozen, replace the provisional translations with the canonical form.

When a port finds a translation that follows the rules but behaves differently from Go:

1. Reduce it to a parity case that reproduces the difference, and mark it as a [known gap](../tests/parity/README.md#known-gaps) until it is fixed.
2. Fix the rule or the library, and delete the known-gap marker.

A frozen form changes only when a port shows that it is wrong or cannot be applied. The change updates the rule, the parity cases that use the form, and the translations that already use it, in the same change.
