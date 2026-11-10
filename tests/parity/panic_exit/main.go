package main

import "fmt"

func main() {
	fmt.Println("before panic")
	panic("boom")
}
