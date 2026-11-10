package main

import "fmt"

func inner() {
	defer fmt.Println("inner deferred")
	panic("first")
}

func outer() {
	defer fmt.Println("outer deferred 1")
	defer func() {
		r := recover()
		fmt.Println("outer recovered:", r)
		panic("second")
	}()
	defer fmt.Println("outer deferred 2")
	inner()
}

func main() {
	defer fmt.Println("main deferred")
	outer()
	fmt.Println("unreachable")
}
