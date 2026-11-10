package main

import "fmt"

func closeTwice(ch chan int) {
	defer func() {
		fmt.Println("recovered:", recover())
		fmt.Println("again:", recover())
	}()
	close(ch)
	close(ch)
}

func main() {
	closeTwice(make(chan int))
	fmt.Println("main continues")
}
