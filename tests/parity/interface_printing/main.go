package main

import "fmt"

func digit(n int) string {
	return string(rune('0' + n))
}

type Point struct {
	X, Y int
}

func (p Point) String() string {
	return "(" + digit(p.X) + "," + digit(p.Y) + ")"
}

type Temp struct {
	C int
}

func (t *Temp) String() string {
	return digit(t.C) + "C"
}

type Fault struct {
	Code int
}

func (f Fault) Error() string {
	return "fault " + digit(f.Code)
}

func (f Fault) String() string {
	return "unused"
}

type Boom struct{}

func (Boom) String() string {
	panic("boom")
}

type Pair struct {
	A Point
	B *Temp
	N int
}

func main() {
	p := Point{X: 1, Y: 2}
	fmt.Println(p, &p)

	// String has a pointer receiver, so a Temp value prints as a struct.
	t := Temp{C: 2}
	fmt.Println(t, &t)

	// Error is preferred to String.
	fmt.Println(Fault{Code: 7})

	// A nil receiver whose method panics prints as <nil>; any other panic is reported.
	var nilTemp *Point
	fmt.Println(nilTemp, Boom{})

	var s fmt.Stringer = p
	var e any
	fmt.Println(s, e, []any{1, "a", nil, 2.5, p, &t})
	fmt.Println(Pair{A: p, B: &t, N: 3}, []Point{p, {X: 3, Y: 4}})
}
