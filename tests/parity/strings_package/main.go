package main

import (
	"fmt"
	"math/bits"
	"strings"
)

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func build() string {
	var b strings.Builder
	fmt.Println(b.Len(), b.String() == "")
	b.Grow(10)
	fmt.Println(b.Len(), b.Cap() >= 10)
	n, err := b.WriteString("héllo")
	fmt.Println(n, err, b.Len())
	b.WriteByte(' ')
	m, err := b.WriteRune(0x263A)
	fmt.Println(m, err)
	b.WriteRune(-1)
	k, err := b.Write([]byte{'!', '?'})
	fmt.Println(k, err, b.Len())
	b.Grow(100)
	fmt.Println(b.Len(), b.Cap() >= 115)
	s := b.String()
	b.Reset()
	fmt.Println(b.Len(), b.String() == "")
	b.WriteString("again")
	return s + " " + b.String()
}

func main() {
	fmt.Printf("%q\n", build())

	// A Builder that was written to and then copied panics on the next write through the copy.
	var a strings.Builder
	a.WriteString("x")
	c := a
	attempt("copy", func() { c.WriteString("y") })
	fmt.Println(c.String(), c.Len())
	var zero strings.Builder
	z := zero
	z.WriteString("fine")
	fmt.Println(z.String())

	attempt("grow", func() {
		var g strings.Builder
		g.Grow(-1)
	})
	attempt("repeat", func() { strings.Repeat("ab", -1) })
	attempt("repeat overflow", func() { strings.Repeat("ab", 1<<62) })
	attempt("replacer", func() { strings.NewReplacer("a", "b", "c") })
	fmt.Printf("%q %q %q\n", strings.Repeat("ab", 0), strings.Repeat("", 5), strings.Repeat("-", 3))
	fmt.Println(strings.IndexFunc("ab,c", func(r rune) bool { return r == ',' }), strings.LastIndexFunc("a,b,c", func(r rune) bool { return r == ',' }))
	l, ok := strings.CutPrefix("prefix-body", "prefix-")
	fmt.Println(l, ok)
	r, ok := strings.CutSuffix("body.go", ".txt")
	fmt.Println(r, ok, bits.UintSize)
}
