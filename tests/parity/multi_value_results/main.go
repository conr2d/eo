package main

import "fmt"

type Point struct {
	X, Y int
}

type Splitter interface {
	Split(n int) (int, int)
}

type Halves struct {
	Offset int
}

func (h Halves) Split(n int) (int, int) {
	return n/2 + h.Offset, n - n/2 + h.Offset
}

func (p *Point) Move(dx, dy int) (int, int) {
	p.X += dx
	p.Y += dy
	return p.X, p.Y
}

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func pair(n int) (int, string) {
	return n * 2, "pair"
}

func forward(n int) (int, string) {
	return pair(n + 1)
}

func three() (int, bool, string) {
	return 1, true, "three"
}

func lookup(k string) (int, bool) {
	if k == "a" {
		return 1, true
	}
	return 0, false
}

func ordered() (int, int) {
	return trace("first", 1), trace("second", 2)
}

func divmod(a, b int) (q, r int) {
	q = a / b
	r = a % b
	return
}

func swapped(a, b int) (x, y int) {
	x = a
	y = b
	return y, x
}

func show(n int, s string) {
	fmt.Println("show", n, s)
}

func main() {
	v, s := pair(3)
	fmt.Println(v, s)

	v, s = forward(4)
	fmt.Println(v, s)

	w, s := pair(5)
	fmt.Println(w, s)

	n, _ := lookup("a")
	_, ok := lookup("b")
	fmt.Println(n, ok)

	n, _ = lookup("z")
	fmt.Println(n)

	_, _ = lookup("a")

	a, _, c := three()
	fmt.Println(a, c)

	if _, found := lookup("a"); found {
		fmt.Println("found a")
	}
	if k, found := lookup("c"); !found {
		fmt.Println("missing c", k)
	}

	x, y := ordered()
	fmt.Println(x, y)

	fmt.Println(divmod(17, 5))
	fmt.Println(swapped(1, 2))

	show(pair(6))

	var sp Splitter = Halves{Offset: 10}
	lo, hi := sp.Split(7)
	fmt.Println(lo, hi)

	pt := Point{X: 1, Y: 2}
	px, py := pt.Move(3, 4)
	fmt.Println(px, py, pt)

	count, label := pair(1)
	inc := func() { count++ }
	inc()
	get := func() string { return label }
	fmt.Println(count, get())

	var ptrs []*int
	for i := 0; i < 2; i++ {
		m, _ := lookup("a")
		m += i
		ptrs = append(ptrs, &m)
	}
	fmt.Println(*ptrs[0], *ptrs[1])

	var keep *int
	{
		q, r := divmod(9, 4)
		keep = &q
		fmt.Println(r)
	}
	fmt.Println(*keep)
}
