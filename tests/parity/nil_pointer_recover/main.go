package main

import "fmt"

type T struct {
	n int
}

func read(p *T) (n int) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r)
			n = -1
		}
	}()
	return p.n
}

func main() {
	fmt.Println(read(&T{n: 3}))
	fmt.Println(read(nil))
}
