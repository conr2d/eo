package main

import "fmt"

func main() {
	s := "abc"
	n := len(s)
	fmt.Println("before")
	fmt.Println(s[n])
	fmt.Println("after")
}
