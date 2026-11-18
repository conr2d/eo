package main

import "fmt"

type Shape interface {
	Area() int
	Scale(k int)
}

type Areaer interface {
	Area() int
}

type Rect struct {
	W, H int
}

func (r Rect) Area() int {
	return r.W * r.H
}

func (r *Rect) Scale(k int) {
	r.W *= k
	r.H *= k
}

type Labeler interface {
	Label() string
}

type Tag struct {
	Name string
}

func (t Tag) Label() string {
	return "#" + t.Name
}

// Card gets Area and Scale from *Rect and Label from its Labeler field.
type Card struct {
	*Rect
	Labeler
}

type Bag struct {
	Items []int
}

func total(shapes []Areaer) int {
	sum := 0
	for _, s := range shapes {
		sum += s.Area()
	}
	return sum
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	r := &Rect{W: 2, H: 3}
	var s Shape = r
	s.Scale(2)
	fmt.Println(s.Area(), r.W, r.H)

	// An interface holds a copy of a value.
	var a Areaer = *r
	r.W = 1
	fmt.Println(a.Area(), r.Area())

	// Areaer has a subset of Shape's methods.
	a = s
	fmt.Println(a.Area(), total([]Areaer{a, Rect{W: 5, H: 5}, s}))

	var none Shape
	var nilRect *Rect
	var typed Shape = nilRect
	fmt.Println(none == nil, typed == nil, typed != nil)
	attempt("nil interface", func() { none.Area() })
	attempt("nil pointer", func() { typed.Area() })

	var x, y any = Rect{W: 1, H: 2}, Rect{W: 1, H: 2}
	fmt.Println(x == y, x == any(Rect{W: 2, H: 1}), x == any(r), any(r) == any(r))
	fmt.Println(any(1) == any(1), any(1) == any(int32(1)), any("a") == any("a"), x == nil)
	attempt("uncomparable", func() {
		var p, q any = Bag{}, Bag{}
		fmt.Println(p == q)
	})
	fmt.Println(any(Bag{}) == any(1))

	c := Card{Rect: &Rect{W: 4, H: 1}, Labeler: Tag{Name: "go"}}
	s = c
	s.Scale(3)
	var l Labeler = c
	fmt.Println(s.Area(), c.W, l.Label(), c.Label())
}
