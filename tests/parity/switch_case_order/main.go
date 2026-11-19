package main

import "fmt"

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func check(name string, b bool) bool {
	fmt.Println("eval", name)
	return b
}

func pick(n int) {
	switch trace("tag", n) {
	case trace("a", 1) + trace("b", 0):
		fmt.Println("first")
	case trace("c", 2), trace("d", 1)*trace("e", 3):
		fmt.Println("second")
	default:
		fmt.Println("default")
	}
}

func test(n int) {
	switch {
	case check("x", n > 5) && check("y", trace("f", n)+trace("g", 1) > 8):
		fmt.Println("big")
	case trace("h", n)+trace("i", 0) == 3:
		fmt.Println("three")
	}
}

func main() {
	pick(1)
	pick(3)
	pick(5)
	test(9)
	test(6)
	test(3)
}
