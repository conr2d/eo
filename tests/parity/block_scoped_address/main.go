package main

import "fmt"

// value is a plain function, not a coroutine, so x is on the C++ stack.
func value() int {
	var p *int
	{
		x := 5
		p = &x
	}
	*p += 1
	return *p
}

// header declares y in an if statement's init, whose C++ scope ends with the statement.
func header() int {
	var p *int
	var f func() int
	if y := 3; y > 0 {
		p = &y
		f = func() int { y++; return y }
	}
	*p += 10
	return f()
}

func main() {
	fmt.Println(value(), header())
}
