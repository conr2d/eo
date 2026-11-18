package main

import (
	"fmt"
	"sync"
)

var mu sync.Mutex
var total int
var launched sync.WaitGroup

func boom() string {
	panic("boom")
}

func partial() (int, string) {
	defer func() {
		fmt.Println("recovered:", recover())
	}()
	return 7, boom()
}

func exchange(a, b int) (x, y int) {
	defer func() {
		x *= 10
	}()
	x = a
	y = b
	return y, x
}

func guarded(n int) (result int, err string) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r)
			err = "failed"
		}
	}()
	if n < 0 {
		panic("negative")
	}
	return n * 2, "ok"
}

func pair(n int) (int, string) {
	return n + 1, "pair"
}

func relay(n int) (int, string) {
	defer fmt.Println("relay done")
	return pair(n)
}

func show(n int, s string) {
	fmt.Println("show", n, s)
}

func add(n int) (int, int) {
	mu.Lock()
	defer mu.Unlock()
	total += n
	return n, total
}

func deferred() {
	n := 1
	defer show(pair(n))
	n = 5
	fmt.Println("n is", n)
}

func main() {
	fmt.Println(partial())
	fmt.Println(exchange(1, 2))
	fmt.Println(guarded(4))
	fmt.Println(guarded(-1))
	fmt.Println(relay(3))
	deferred()

	n, t := add(2)
	fmt.Println(n, t)
	n, t = add(3)
	fmt.Println(n, t)

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		v, s := pair(8)
		fmt.Println("goroutine", v, s)
	}()
	wg.Wait()

	wg.Add(1)
	go report(&wg, 9)
	wg.Wait()

	launched.Add(1)
	go show2(pair(10))
	launched.Wait()
}

func report(wg *sync.WaitGroup, n int) {
	defer wg.Done()
	a, b := add(n)
	fmt.Println("report", a, b)
}

func show2(n int, s string) {
	defer launched.Done()
	fmt.Println("show2", n, s)
}
