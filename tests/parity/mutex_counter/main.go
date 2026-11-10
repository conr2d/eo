package main

import (
	"fmt"
	"sync"
)

var (
	mu    sync.Mutex
	count int
	wg    sync.WaitGroup
)

func inc() {
	mu.Lock()
	defer mu.Unlock()
	count++
}

func worker(n int) {
	for i := 0; i < n; i++ {
		inc()
	}
	wg.Done()
}

func main() {
	wg.Add(3)
	go worker(1000)
	go worker(1000)
	go worker(1000)
	wg.Wait()
	fmt.Println("count =", count)
}
