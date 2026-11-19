package main

import "fmt"

type Box struct {
	v int
}

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func cond(name string, b bool) bool {
	fmt.Println("eval", name)
	return b
}

func add(a, b int) int {
	return a + b
}

func box(name string, b *Box) *Box {
	fmt.Println("eval", name)
	return b
}

func (b *Box) Add(x, y int) int {
	b.v += x + y
	return b.v
}

func (b *Box) Next(x int) *Box {
	b.v += x
	return b
}

func pair() (int, int) {
	return trace("p0", 1), trace("p1", 2)
}

func main() {
	// Arguments and operands.
	fmt.Println(trace("a", 1), trace("b", 2))
	x := trace("c", 3) + trace("d", 4)*trace("e", 5)
	fmt.Println(x)
	fmt.Println(add(trace("f", 6), trace("g", 7)))
	fmt.Println(add(trace("h", 8), 1))
	u, w := pair()
	fmt.Println(u, w)

	// Receivers and selectors.
	b := &Box{}
	fmt.Println(box("recv", b).Add(trace("i", 1), trace("j", 2)))
	*box("deref", b) = Box{trace("k", 10)}
	box("field", b).v = trace("l", 20) + 1
	box("op", b).v += trace("m", 5)
	fmt.Println(b.v)
	fmt.Println(box("chain", b).Next(trace("q", 1)).Next(trace("r", 2)).v)

	// Short-circuit operands.
	if cond("l1", true) && add(trace("s", 1), trace("t", 2)) == 3 {
		fmt.Println("and")
	}
	if cond("l2", false) && add(trace("never", 1), trace("never", 2)) == 3 {
		fmt.Println("never")
	} else {
		fmt.Println("short")
	}
	ok := cond("l3", false) || add(trace("y", 1), trace("z", 2)) == 3
	fmt.Println(ok)
	fmt.Println(cond("l4", true) || cond("never", true), trace("after", 0))

	// Positions that no statement can precede.
	if trace("if", 1) == 0 {
		fmt.Println("never")
	} else if trace("elif1", 2)+trace("elif2", 3) == 5 {
		fmt.Println("else if")
	}
	if n := trace("init", 4); trace("c1", n)+trace("c2", 1) == 5 {
		fmt.Println("init", n)
	} else {
		fmt.Println("never", n)
	}
	for i := 0; i < trace("lim", 3)+trace("step", 0)*i; i++ {
		if i == 1 {
			continue
		}
		fmt.Println("loop", i)
	}

	// An op-assignment that translates to a helper.
	xs := []int{12}
	xs[0] /= trace("div", 4)
	fmt.Println(xs)
}
