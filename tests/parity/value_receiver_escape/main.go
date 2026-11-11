package main

import "fmt"

type T struct {
	n int
}

// Start launches a goroutine that uses the receiver after Start returns.
func (t T) Start(results chan int) {
	go func() {
		results <- t.n * 2
	}()
}

// addr returns the address of a field of the receiver, which is moved to heap.
func (t T) addr() *int {
	return &t.n
}

func main() {
	results := make(chan int)
	T{n: 4}.Start(results)
	fmt.Println(<-results)
	p := T{n: 7}.addr()
	*p++
	fmt.Println(*p)
}
