package main

import "fmt"

func main() {
	x := 10
	defer fmt.Println("first deferred, x =", x)
	x = 20
	defer fmt.Println("second deferred, x =", x)
	x = 30
	fmt.Println("x =", x)
}
