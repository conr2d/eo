package main

import "fmt"

type Node struct {
	p    *int
	next *Node
}

func show(head *Node) {
	for n := head; n != nil; n = n.next {
		fmt.Println(*n.p)
		*n.p += 100
	}
	for n := head; n != nil; n = n.next {
		fmt.Println(*n.p)
	}
}

func steps() {
	var head *Node
	for i := 0; i < 8; i++ {
		head = &Node{p: &i, next: head}
		if i%3 == 1 {
			i++
			continue
		}
		i++
	}
	show(head)
}

func pairs() {
	var head *Node
	for i, j := 0, 10; i < j; {
		head = &Node{p: &i, next: head}
		head = &Node{p: &j, next: head}
		i += 3
		j -= 2
	}
	show(head)
}

func main() {
	steps()
	pairs()
}
