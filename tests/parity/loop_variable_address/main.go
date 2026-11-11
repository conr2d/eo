package main

import "fmt"

type Node struct {
	p    *int
	next *Node
}

func main() {
	var head *Node
	for i := 0; i < 8; i++ {
		if i%3 == 1 {
			i++
			continue
		}
		head = &Node{p: &i, next: head}
		i++
	}
	for n := head; n != nil; n = n.next {
		fmt.Println(*n.p)
		*n.p += 100
	}
	for n := head; n != nil; n = n.next {
		fmt.Println(*n.p)
	}
}
