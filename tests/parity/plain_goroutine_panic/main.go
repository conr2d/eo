package main

import (
	"fmt"
	"time"
)

func work() {
	panic("boom")
}

func main() {
	fmt.Println("start")
	go work()
	time.Sleep(time.Second)
	fmt.Println("unreachable")
}
