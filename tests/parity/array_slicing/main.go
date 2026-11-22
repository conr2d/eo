package main

import "fmt"

// The slice outlives the function, so the array is moved to heap.
func digits() []int {
	a := [4]int{1, 2, 3, 4}
	s := a[1:3]
	a[1] = 20
	return s
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	// A slice of an array shares its elements, and its capacity runs to the end of the array.
	a := [5]int{0, 1, 2, 3, 4}
	s := a[1:3]
	s[0] = 10
	a[2] = 20
	fmt.Println(a, s, len(s), cap(s))
	t := a[:]
	u := a[2:4:4]
	t[4] = 40
	fmt.Println(t, u, len(u), cap(u))
	s = append(s, 30)
	fmt.Println(a, s)
	u = append(u, 50)
	u[0] = 0
	fmt.Println(a, u)
	n := copy(a[3:], []int{7, 8, 9})
	fmt.Println(n, a)

	d := digits()
	fmt.Println(d, len(d), cap(d))

	// A slice of a pointer to an array, and of an array field.
	p := &[3]string{"a", "b", "c"}
	ps := p[1:]
	ps[0] = "B"
	fmt.Println(*p, ps)
	type box struct {
		buf [4]byte
	}
	var bx box
	copy(bx.buf[:], "hey!")
	fmt.Println(bx.buf, string(bx.buf[:3]))

	// Out-of-range bounds and a nil pointer panic.
	lo, hi, mx := 2, 6, 7
	attempt("high", func() { fmt.Println(a[:hi]) })
	attempt("low", func() { fmt.Println(a[lo+2 : 3]) })
	attempt("max", func() { fmt.Println(a[1:2:mx]) })
	attempt("high3", func() { fmt.Println(a[0:lo+2:3]) })
	var np *[3]int
	attempt("nil", func() { fmt.Println(np[:]) })
	attempt("nil len", func() { fmt.Println(len(np), cap(np)) })
	attempt("index", func() { fmt.Println(np[lo]) })

	// Converting a slice to an array copies its first elements.
	src := []int{1, 2, 3, 4}
	arr := [3]int(src)
	src[0] = 100
	fmt.Println(arr, src)
	var empty [0]int = [0]int(src)
	fmt.Println(empty, [0]int([]int(nil)))
	attempt("convert", func() { fmt.Println([5]int(src)) })
	attempt("convert nil", func() { fmt.Println([1]int([]int(nil))) })
}
