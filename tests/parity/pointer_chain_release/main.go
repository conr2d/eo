package main

import "fmt"

type Node struct {
	next *Node
	v    int
}

func main() {
	var head *Node
	for i := 0; i < 1000000; i++ {
		head = &Node{next: head, v: i}
	}
	fmt.Println(head.v, head.next.v)
	head = nil
	fmt.Println("freed")
}
