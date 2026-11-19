package main

import (
	"errors"
	"fmt"
)

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func classify(n int) string {
	switch n {
	case 0:
		return "zero"
	case 1, 3, 5, 7, 9:
		return "odd digit"
	default:
		return "other"
	case 2, 4, 6, 8:
		return "even digit"
	}
}

func word(s string) int {
	switch s {
	case "one":
		return 1
	case "two":
		return 2
	}
	return -1
}

func sign(n int) string {
	switch {
	case n < 0:
		return "negative"
	case n == 0:
		return "zero"
	default:
		return "positive"
	}
}

func lookup(key string, errNotFound error) (int, error) {
	if key == "a" {
		return 1, nil
	}
	if key == "b" {
		return 0, fmt.Errorf("lookup %s: %w", key, errNotFound)
	}
	return 0, errors.New("bad key")
}

func handle(key string, errNotFound error) {
	v, err := lookup(key, errNotFound)
	switch {
	case err == nil:
	case errors.Is(err, errNotFound):
		fmt.Println(key, "missing")
		return
	default:
		fmt.Println(key, "error:", err)
		return
	}
	fmt.Println(key, "=", v)
}

func describe(v any) string {
	switch v {
	case nil:
		return "nil"
	case 1:
		return "int one"
	case "one":
		return "string one"
	case byte(1):
		return "byte one"
	}
	return fmt.Sprint("unmatched ", v)
}

func main() {
	for i := 0; i < 11; i++ {
		fmt.Println(i, classify(i))
	}
	fmt.Println(word("one"), word("two"), word("three"))
	fmt.Println(sign(-4), sign(0), sign(9))
	errNotFound := errors.New("not found")
	handle("a", errNotFound)
	handle("b", errNotFound)
	handle("c", errNotFound)
	fmt.Println(describe(nil), describe(1), describe("one"), describe(byte(1)), describe(2.5))

	// The tag is evaluated once, and the case expressions only until one matches.
	switch trace("tag", 2) {
	case trace("a", 1), trace("b", 2), trace("c", 3):
		fmt.Println("matched b")
	case trace("d", 2):
		fmt.Println("not reached")
	}
	switch trace("tag", 9) {
	case trace("a", 1):
		fmt.Println("not reached")
	default:
		fmt.Println("default after all cases")
	case trace("b", 2):
		fmt.Println("not reached")
	}

	// The tag is a copy: a case expression that changes the variable does not change it.
	x := 1
	switch x {
	case func() int { x = 2; return 2 }():
		fmt.Println("not reached")
	case 1:
		fmt.Println("tag kept", x)
	}

	// Init statements, in tagged and tagless switches.
	switch y := trace("init", 3); y * 2 {
	case 6:
		fmt.Println("y doubled", y)
	}
	switch y := trace("init", 5); {
	case y > 4:
		y := y * 10
		fmt.Println("shadowed", y)
	}

	// A switch with only a default clause still evaluates its tag.
	switch trace("lonely", 0) {
	default:
		fmt.Println("only default")
	}
	switch trace("empty", 0) {
	}

	// Nested switches, and continue inside a clause.
	for i := 0; i < 4; i++ {
		switch i % 2 {
		case 0:
			switch i {
			case 0:
				fmt.Println("zero")
			default:
				fmt.Println("even", i)
			}
		case 1:
			if i == 3 {
				continue
			}
			fmt.Println("odd", i)
		}
		fmt.Println("end", i)
	}
}
