package main

import "fmt"

var o = Outer{In: Inner{N: derived}, Tag: "xyz"}

func init() {
	fmt.Println("init main.go, o =", o)
}

func twice(n int) int {
	return n * 2
}

func main() {
	fmt.Println("main, o.Sum() =", o.Sum())
}
