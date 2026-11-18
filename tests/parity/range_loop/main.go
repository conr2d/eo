package main

import "fmt"

func next() []int {
	fmt.Println("next called")
	return []int{1, 2, 3}
}

func g() int {
	fmt.Println("g called")
	return 1
}

func main() {
	// range over an integer
	for i := range 3 {
		fmt.Println("int", i)
	}
	var n uint8 = 2
	for i := range n {
		fmt.Println("uint8", i)
	}
	count := 0
	for range 4 {
		count++
	}
	fmt.Println("count", count)
	for i := range 3 {
		i += 10
		fmt.Println("write", i)
	}
	for i := range -2 {
		fmt.Println("negative", i)
	}
	j := 7
	for j = range 3 {
	}
	fmt.Println("assigned", j)
	for j = range 0 {
	}
	fmt.Println("unchanged", j)

	// range over a slice: the slice is evaluated once and each element is read when its iteration starts
	s := []int{10, 20, 30}
	for i, v := range s {
		if i == 0 {
			s[2] = 99
			s = append(s, 40)
			s[1] = -1
		}
		fmt.Println("slice", i, v)
	}
	fmt.Println(s)
	u := make([]int, 2, 4)
	for i := range u {
		u = append(u, i)
	}
	fmt.Println(u, len(u))
	for _, v := range next() {
		fmt.Println("once", v)
	}
	var none []string
	for i := range none {
		fmt.Println("never", i)
	}
	words := []string{"a", "b"}
	for _, w := range words {
		w += "!"
		fmt.Println(w)
	}
	fmt.Println(words)
	var k int
	var w string
	for k, w = range words {
	}
	fmt.Println(k, w)
	for i, v := range s {
		sum := func() int { return i + v }
		fmt.Println("captured", sum())
	}
	for _, v := range []int{5} {
		inc := func() { v++ }
		inc()
		fmt.Println("by ref", v)
	}

	// break and continue
	for i := range 3 {
		if i == 1 {
			continue
		}
		for _, v := range s {
			if v > 50 {
				break
			}
			fmt.Println("nested", i, v)
		}
	}

	// range over a string decodes UTF-8
	for i, r := range "a\u00e9\xff\u20ac" {
		fmt.Println("string", i, r)
	}
	str := "h\u00e9llo"
	for i := range str {
		if i == 0 {
			str = "x"
		}
		fmt.Println("index", i)
	}
	fmt.Println(str)
	for range "abc" {
		count++
	}
	fmt.Println("count", count)
	for i, b := range []byte("hi") {
		fmt.Println("byte", i, b)
	}

	// a nested call in the range expression runs once, before the loop
	for _, v := range next()[g():] {
		fmt.Println("tail", v)
	}

	// an untyped rune constant ranges over rune values
	for r := range 'c' - 'a' {
		fmt.Println("rune", string(r+'a'))
	}

	// a body may redeclare a clause variable
	for _, v := range s {
		v := v * 10
		fmt.Println("shadow", v)
	}
	for i := 0; i < 2; i++ {
		i := i + 100
		fmt.Println("shadow", i)
	}

	// the same variable on both sides is assigned the index, then the element
	var x int
	for x, x = range []int{7, 8} {
	}
	fmt.Println("twice", x)
}
