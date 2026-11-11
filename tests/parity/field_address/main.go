package main

import "fmt"

type Pair struct {
	a int
	b int
}

func main() {
	p := Pair{a: 1, b: 2}
	pa := &p.a
	done := make(chan bool)
	go func() {
		*pa = 10
		p.b = 20
		done <- true
	}()
	<-done
	fmt.Println(p.a, p.b, *pa)
}
