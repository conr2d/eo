package main

import "fmt"

type Point struct {
	X, Y int
}

type Grid struct {
	Name  string
	Cells [3]int
}

func bump(a [3]int) [3]int {
	a[0]++
	return a
}

func get(a [4]int, i int) (v int) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r)
			v = -1
		}
	}()
	return a[i]
}

func main() {
	// Assignment, arguments, and results copy an array.
	a := [3]int{1, 2, 3}
	b := a
	b[1] = 20
	c := bump(a)
	fmt.Println(a, b, c, len(a), cap(a))
	fmt.Println(a == [3]int{1, 2, 3}, a != b, a == c)

	// Missing elements are zero, [...] counts the elements, and keyed elements leave zero gaps.
	var z [4]int
	few := [5]string{"x", "y"}
	counted := [...]float64{1.5, 2.5}
	keyed := [6]int{1, 4: 40, 50, 2: 20}
	tail := [...]string{3: "d", 1: "b"}
	ks := []int{2: 7, 9}
	fmt.Println(z, len(counted), counted, keyed, len(tail), ks, len(ks), cap(ks))
	fmt.Printf("%q %q\n", few, tail)

	// An index out of range panics.
	fmt.Println(get([4]int{5, 6, 7, 8}, 3), get([4]int{}, 4), get([4]int{}, -1))

	// Arrays of arrays and of structs, and a struct with an array field.
	var m [2][3]int
	m[1][2] = 9
	row := m[1]
	row[0] = 1
	ps := [2]Point{{1, 2}, {3, 4}}
	ps[1].X = 30
	g := Grid{Name: "g", Cells: [3]int{7, 8, 9}}
	h := g
	h.Cells[0] = 70
	fmt.Println(m, row, ps, g, h, g == h, g == Grid{"g", [3]int{7, 8, 9}})
	fmt.Printf("%v %v %d\n", ps, g, m)

	// The address of an element keeps referring to the array.
	p := &a[2]
	*p = 30
	q := &m[0]
	q[1] = 5
	fmt.Println(a, m)

	// range over an array iterates over a copy of it, and range over a pointer to an array does not.
	arr := [3]int{1, 2, 3}
	for i, v := range arr {
		arr[2] = 100
		fmt.Printf("%d:%d ", i, v)
	}
	fmt.Println(arr)
	arr = [3]int{1, 2, 3}
	for i, v := range &arr {
		arr[2] = 100
		fmt.Printf("%d:%d ", i, v)
	}
	fmt.Println()
	var nilArr *[2]int
	for i := range nilArr {
		fmt.Printf("%d ", i)
	}
	fmt.Println(len(nilArr))

	// Printing.
	bs := [4]byte{0xde, 0xad, 'h', 'i'}
	fmt.Println(bs, &arr, [0]int{})
	fmt.Printf("%v %x %X %s %q %T %T\n", bs, bs, bs, bs[2:], bs[2:], bs, m)
	fmt.Printf("%v %d %T\n", &ps, &m, &m)

	// Arrays as map keys and in interfaces.
	seen := map[[2]int]string{{1, 2}: "a", {0, 5}: "b"}
	seen[[2]int{1, 2}] = "c"
	fk := map[[2]float64]int{{0, 1}: 1}
	fk[[2]float64{-0.0, 1}]++
	fmt.Println(seen, len(seen), seen[[2]int{0, 5}], fk)
	var x, y any = [2]int{1, 2}, [2]int{1, 2}
	fmt.Println(x == y, x == any([2]int{2, 1}), x)
	if v, ok := x.([2]int); ok {
		fmt.Println(v[1])
	}
	_, ok := x.([3]int)
	fmt.Println(ok)
}
