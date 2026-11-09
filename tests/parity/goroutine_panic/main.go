package main

import "fmt"

func main() {
	fmt.Println("start")
	done := make(chan bool)
	go func() {
		panic("boom")
	}()
	<-done
	fmt.Println("unreachable")
}
