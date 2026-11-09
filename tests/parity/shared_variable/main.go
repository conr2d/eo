package main

import "fmt"

func main() {
	x := 0
	done := make(chan bool)
	go func() {
		x = 42
		done <- true
	}()
	<-done
	fmt.Println("x =", x)
}
