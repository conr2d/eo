package main

import (
	"fmt"
	"sync"
)

var mu sync.Mutex

// waiter blocks in Lock while main holds mu and is itself blocked on a channel.
func waiter(done chan string) {
	mu.Lock()
	mu.Unlock()
	done <- "waiter locked"
}

func sender(ch chan int) {
	ch <- 1
}

func main() {
	ch := make(chan int)
	done := make(chan string)
	mu.Lock()
	go waiter(done)
	go sender(ch)
	v := <-ch
	fmt.Println("holder received", v)
	mu.Unlock()
	fmt.Println(<-done)
}
