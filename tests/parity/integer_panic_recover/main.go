package main

import "fmt"

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, recover())
	}()
	f()
}

func main() {
	zero := 0
	attempt("div", func() { fmt.Println(1 / zero) })
	attempt("rem", func() { fmt.Println(int8(5) % int8(zero)) })
	attempt("div_assign", func() {
		var b uint8
		b /= b
		fmt.Println(b)
	})
	attempt("shift", func() {
		n := -1
		fmt.Println(1 << n)
	})
	attempt("shift_assign", func() {
		n := int8(-3)
		x := uint64(8)
		x >>= n
		fmt.Println(x)
	})
	attempt("ok", func() { fmt.Println(7/(zero+2), 1<<(zero+64)) })
}
