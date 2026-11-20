package main

import "fmt"

var before = trace("before")
var failed = boom()
var after = trace("after")

func trace(label string) string {
	fmt.Println("init", label)
	return label
}

func boom() int {
	defer fmt.Println("deferred in boom")
	panic("initializer failed")
}

func init() {
	fmt.Println("init function")
}

func main() {
	fmt.Println("main", before, failed, after)
}
