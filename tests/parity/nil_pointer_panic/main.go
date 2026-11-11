package main

import "fmt"

func main() {
	var p *int
	fmt.Println("before")
	fmt.Println(*p)
	fmt.Println("after")
}
