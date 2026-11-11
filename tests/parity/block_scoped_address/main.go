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

func main() {
	fmt.Println(value())
}
