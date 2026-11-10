package main

import (
	"fmt"
	"os"
	"time"
)

func main() {
	defer fmt.Println("deferred")
	fmt.Println("start")
	go func() {
		fmt.Println("exiting")
		os.Exit(4)
	}()
	time.Sleep(time.Second)
	fmt.Println("unreachable")
}
