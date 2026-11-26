package main

import (
	"errors"
	"fmt"
)

type pair struct {
	a int
	b int
}

var kept *int

func trace(name string, v int) int {
	fmt.Println("eval", name)
	return v
}

func fib(n int) int {
	a := 0
	b := 1
	for i := 0; i < n; i++ {
		a, b = b, a+b
	}
	return a
}

func reverse(s []int) {
	for i, j := 0, len(s)-1; i < j; i, j = i+1, j-1 {
		t := s[i]
		s[i] = s[j]
		s[j] = t
	}
}

func lookup(m map[string]int, k string) (int, error) {
	var v int
	var err error
	v, err = m[k], nil
	if v == 0 {
		v, err = -1, errors.New("missing "+k)
	}
	return v, err
}

func partial(s []int) (a int, b int) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r, a, b)
		}
	}()
	a = 1
	b = 2
	a, b = 10, s[5]
	return a, b
}

func main() {
	fmt.Println(fib(10), fib(50))

	x := 1
	y := 2
	x, y = y, x
	fmt.Println(x, y)
	x, y = y+10, x*10
	fmt.Println(x, y)
	x, y = trace("first", y), trace("second", x)
	fmt.Println(x, y)
	_, y = trace("blank", 0), 7
	fmt.Println(x, y)

	var f float64
	var u uint8
	var r rune
	f, u, r = 1, 255, 'z'
	fmt.Println(f, u, r)

	var e error
	var p *pair
	e, p = nil, &pair{1, 2}
	fmt.Println(e == nil, *p)
	e, p = errors.New("boom"), nil
	fmt.Println(e, p == nil)

	var v any
	var s string
	v, s = 3, "three"
	fmt.Println(v, s)

	z := 5
	kept = &z
	z, x = x, z
	fmt.Println(z, x, *kept)

	nums := []int{1, 2, 3, 4, 5}
	reverse(nums)
	fmt.Println(nums)

	m := map[string]int{"a": 1}
	fmt.Println(lookup(m, "a"))
	fmt.Println(lookup(m, "b"))

	fmt.Println(partial([]int{1, 2}))
	fmt.Println(partial([]int{0, 0, 0, 0, 0, 6}))
}
