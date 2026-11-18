package main

import "fmt"

// Outer holds an Inner by value before Inner is declared.
type Outer struct {
	Name  string
	In    Inner
	Count *int
	Items []int
}

type Inner struct {
	A, B int
}

// Parent and Child refer to each other through pointers.
type Parent struct {
	Name  string
	First *Child
}

type Child struct {
	Name   string
	Parent *Parent
	Next   *Child
}

func bump(o Outer) Outer {
	o.Name += "!"
	o.In.A++
	*o.Count++
	o.Items[0]++
	return o
}

func main() {
	n := 1
	a := Outer{Name: "a", In: Inner{1, 2}, Count: &n, Items: []int{10, 20}}
	b := a
	b.Name = "b"
	b.In.B = 9
	fmt.Println(a.Name, a.In, b.Name, b.In)

	c := bump(a)
	fmt.Println(a.Name, a.In, *a.Count, a.Items)
	fmt.Println(c.Name, c.In, *c.Count, c.Items)
	fmt.Println(a.Count == c.Count, n)

	x := Inner{1, 2}
	y := Inner{A: 1, B: 2}
	z := Inner{2, 1}
	fmt.Println(x == y, x != y, x == z, x != z)

	p := &Parent{Name: "p"}
	c1 := &Child{Name: "c1", Parent: p}
	c2 := &Child{Name: "c2", Parent: p, Next: c1}
	p.First = c2
	for c := p.First; c != nil; c = c.Next {
		fmt.Println(c.Name, c.Parent.Name, c.Parent == p)
	}
	defer func() {
		p.First = nil // Eo never frees a reference cycle (#80).
	}()

	k1 := Child{Name: "k", Parent: p}
	k2 := Child{Name: "k", Parent: p}
	k3 := Child{Name: "k", Parent: &Parent{Name: "p"}}
	fmt.Println(k1 == k2, k1 == k3)

	arr := []Inner{{1, 1}, {2, 2}}
	first := arr[0]
	first.A = 100
	arr[1].B = 200
	fmt.Println(first, arr)
}
