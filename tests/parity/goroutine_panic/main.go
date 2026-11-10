package main

import (
	"fmt"
	"time"
)

func main() {
	fmt.Println("start")
	go func() {
		panic("boom")
	}()
	// Sleep rather than block on a channel: Go's deadlock detector would also
	// exit 2 with the same output, which would hide a discarded panic.
	time.Sleep(time.Second)
	fmt.Println("unreachable")
}
