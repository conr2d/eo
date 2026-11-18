package main

import (
	"fmt"
	"sync"
)

type Config struct {
	Depth int
}

type Hash struct {
	Lo, Hi int
}

// Node has a field named after its type and a method named after another type.
type Node struct {
	Config Config
	Value  int
}

func Size(n int) int {
	return n * 10
}

// Hash is a method named after the type Hash, and Size calls the package function Size.
func (n Node) Hash() Hash {
	return Hash{Lo: n.Value, Hi: n.Config.Depth}
}

func (n *Node) Size() int {
	return Size(n.Value) + n.Config.Depth
}

func (n *Node) Reset() {
	*n = Node{Config: Config{Depth: 1}}
}

type point struct {
	x, y int
}

func mk(x int) point {
	return point{x, x + 1}
}

func main() {
	n := Node{Config: Config{Depth: 2}, Value: 3}
	fmt.Println(n.Hash(), n.Size(), n)
	n.Reset()
	fmt.Println(n, n.Hash() == Hash{0, 1})

	// p is moved to heap while the variable point hides the type point.
	point := 1
	p := mk(point)
	q := &p
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		q.x = 7
	}()
	wg.Wait()
	fmt.Println(point, p, *q)

	{
		mk := mk(4)
		fmt.Println(mk)
	}
	type local struct{ a int }
	fmt.Println(local{a: 5})
}
