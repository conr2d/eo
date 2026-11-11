package main

import "fmt"

func main() {
	x := 0
	y := 5
	inc := func() {
		x++
	}
	get := func() int {
		return x + y
	}
	inc()
	inc()
	fmt.Println(x, get())
}
