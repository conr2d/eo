package main

import "fmt"

type Relay struct {
	name string
	on   func(int) int
}

func (r *Relay) Fire(v int) int {
	return r.on(v)
}

func twice(n int) int {
	return n * 2
}

func run(f func(int) int, v int) int {
	return f(v)
}

func sender(ch chan int) func(int) {
	return func(v int) {
		ch <- v
	}
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	ch := make(chan int, 1)
	send := func(v int) int {
		ch <- v
		return v + 1
	}
	fmt.Println("run", run(send, 3), <-ch)
	fmt.Println("run", run(twice, 4))

	r := &Relay{name: "relay"}
	fmt.Println("relay", r.name, r.on == nil)
	r.on = send
	fmt.Println("fire", r.Fire(5), <-ch)
	r.on = twice
	fmt.Println("fire", r.Fire(6))

	fire := r.Fire
	r.on = send
	fmt.Println("method value", fire(7), <-ch)

	out := make(chan int)
	push := sender(out)
	go push(8)
	fmt.Println("go", <-out)

	b := &Relay{}
	started := make(chan bool)
	gate := make(chan int)
	done := make(chan int)
	b.on = func(v int) int {
		started <- true
		return <-gate + v
	}
	go func() {
		done <- b.Fire(1)
	}()
	<-started
	b.on = nil
	gate <- 41
	fmt.Println("replaced while blocked", <-done, b.on == nil)

	attempt("nil coroutine", func() {
		fmt.Println(b.Fire(2))
	})
}
