package main

import (
	"fmt"
	"sync"
)

func main() {
	// A fatal error runs no deferred calls and cannot be recovered.
	defer func() {
		fmt.Println("recovered:", recover())
	}()
	var mu sync.Mutex
	fmt.Println("unlocking")
	mu.Unlock()
	fmt.Println("unreachable")
}
