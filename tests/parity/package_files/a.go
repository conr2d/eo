package main

import "fmt"

// Outer holds Inner, which b.go declares, by value.
type Outer struct {
	In  Inner
	Tag string
}

// derived depends on base, which b.go declares, so base is initialized first.
var derived = trace("derived", base*2)

func init() {
	fmt.Println("init a.go #1, derived =", derived)
}

func init() {
	fmt.Println("init a.go #2, twice =", twice(derived))
}
