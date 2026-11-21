package main

import "fmt"

type Inner struct {
	N int
}

var base = trace("base", 10)

func init() {
	fmt.Println("init b.go, base =", base)
}

func trace(name string, v int) int {
	fmt.Println("var", name, "=", v)
	return v
}

// Sum is a method of Outer, which a.go declares.
func (o Outer) Sum() int {
	return o.In.N + len(o.Tag)
}
