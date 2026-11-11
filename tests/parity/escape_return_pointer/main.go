package main

import "fmt"

func newCounter() *int {
	n := 0
	return &n
}

func incr(p *int) {
	*p++
}

func main() {
	a := newCounter()
	b := newCounter()
	incr(a)
	incr(a)
	incr(b)
	fmt.Println(*a, *b, a == b)
	c := a
	incr(c)
	fmt.Println(*a, a == c)
}
