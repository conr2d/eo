package main

import (
	"errors"
	"fmt"
)

type Box struct {
	n int
}

func (b *Box) String() string {
	return fmt.Sprint("box ", b.n)
}

// A compile-time check that *Box implements fmt.Stringer.
var _ fmt.Stringer = (*Box)(nil)

// total depends on first and second, which are declared after it.
var total = sum(first, second)
var first = trace("first", 1)
var second = trace("second", first+1)

var limit = 3
var name = "eo"
var count int
var ErrEmpty = errors.New("empty")
var twice = func(n int) int { return n * 2 }
var q, r = divmod(17, 5)
var _ = trace("blank", 0)
var p = &count
var box = &Box{n: limit}

// echoed blocks on a channel, so its initializer is a coroutine call.
var ch = make(chan int, 1)
var echoed = echo(7)

var ready = make(chan string)

func trace(label string, n int) int {
	fmt.Println("init", label, n)
	return n
}

func sum(a, b int) int {
	return a + b
}

func divmod(a, b int) (int, int) {
	return a / b, a % b
}

func echo(n int) int {
	ch <- n
	return <-ch
}

func check(n int) error {
	if n == 0 {
		return ErrEmpty
	}
	return nil
}

func init() {
	fmt.Println("init 1:", total, count)
	count++
}

func init() {
	fmt.Println("init 2:", count)
	go func() {
		ready <- "sent from init"
	}()
	*p += 10
}

func main() {
	fmt.Println("main:", total, first, second, limit, name, count)
	fmt.Println(twice(limit), q, r, echoed, box)
	fmt.Println(check(0) == ErrEmpty, errors.Is(check(0), ErrEmpty), check(1))
	fmt.Println(<-ready)
}
