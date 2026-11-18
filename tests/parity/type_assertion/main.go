package main

import "fmt"

type Shape interface {
	Area() int
}

type Namer interface {
	Name() string
}

type Rect struct {
	W, H int
}

func (r Rect) Area() int {
	return r.W * r.H
}

type Circle struct {
	R int
}

func (c *Circle) Area() int {
	return 3 * c.R * c.R
}

func (c *Circle) Name() string {
	return "circle"
}

func describe(v any) {
	switch x := v.(type) {
	case nil:
		fmt.Println("nil")
	case int, string:
		fmt.Println("int or string", x)
	case Rect:
		fmt.Println("rect", x.W, x.H)
	case Namer:
		fmt.Println("namer", x.Name())
	case Shape:
		fmt.Println("shape", x.Area())
	default:
		fmt.Println("other")
	}
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	var s Shape = Rect{W: 2, H: 3}
	r := s.(Rect)
	fmt.Println(r.W, r.H)

	c, ok := s.(*Circle)
	fmt.Println(c == nil, ok)

	s = &Circle{R: 1}
	c, ok = s.(*Circle)
	fmt.Println(c.R, ok)

	n, ok := s.(Namer)
	fmt.Println(n.Name(), ok)

	_, ok = any(Rect{}).(Namer)
	fmt.Println(ok)

	var a any = 7
	i := a.(int)
	fmt.Println(i + 1)

	attempt("wrong type", func() { fmt.Println(a.(string)) })
	attempt("nil", func() {
		var e any
		fmt.Println(e.(int))
	})
	attempt("named interface", func() {
		var e Shape = Rect{}
		fmt.Println(e.(*Circle))
	})
	attempt("missing method", func() { fmt.Println(a.(Shape)) })
	attempt("nil to interface", func() {
		var e Shape
		fmt.Println(e.(Namer))
	})

	describe(nil)
	describe(1)
	describe("s")
	describe(Rect{W: 1, H: 1})
	describe(&Circle{R: 2})
	describe(2.5)
}
