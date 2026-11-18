package main

import "fmt"

type Point struct {
	X, Y int
}

type Labeled struct {
	Name  string
	At    Point
	Tags  []string
	Next  *Labeled
	Valid bool
}

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func show(n int, p Point) {
	fmt.Println("show", n, p)
}

func main() {
	var zero Labeled
	fmt.Println(zero)
	fmt.Println(zero.At == Point{}, zero.Name == "")

	p := Point{1, 2}
	q := Point{Y: 5}
	fmt.Println(p, q, Point{})

	l := Labeled{Name: "a", At: Point{3, 4}, Tags: []string{"x", "y"}}
	fmt.Println(l)
	fmt.Println(l.At.X, len(l.Tags), l.Next == nil, l.Valid)

	ps := []Point{{1, 2}, {Y: 3}, {}}
	fmt.Println(ps, len(ps))

	pp := []*Point{{5, 6}, nil}
	fmt.Println(*pp[0], pp[1] == nil)

	head := &Labeled{Name: "head", Next: &Labeled{Name: "tail"}}
	fmt.Println(head.Name, head.Next.Name, head.Next.Next == nil)
	fmt.Println(&Point{7, 8})

	r := Point{Y: trace("y", 10), X: trace("x", 20)}
	fmt.Println(r)

	show(trace("n", 1), Point{Y: trace("y", 2), X: trace("x", 3)})

	in := Point{X: trace("x", 4), Y: trace("y", 5)}
	fmt.Println(in)

	e := struct{}{}
	fmt.Println(e, e == struct{}{})
}
