package main

import (
	"fmt"
	"time"
)

func main() {
	go func() {
		for {
			time.Sleep(time.Millisecond)
		}
	}()
	time.Sleep(10 * time.Millisecond)
	fmt.Println("main returns")
}
