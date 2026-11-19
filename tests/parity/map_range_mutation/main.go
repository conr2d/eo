package main

import "fmt"

func main() {
	m := map[int]int{1: 10, 2: 20, 3: 30, 4: 40}
	iterations := 0
	for k := range m {
		iterations++
		for _, other := range []int{1, 2, 3, 4} {
			if other != k {
				delete(m, other)
			}
		}
	}
	fmt.Println(iterations, len(m))

	u := map[string]int{"a": 0, "b": 0}
	total := 0
	for k, v := range u {
		total += v
		if k == "a" {
			u["b"] = 5
		} else {
			u["a"] = 5
		}
	}
	fmt.Println(total)

	c := map[int]bool{1: true, 2: true, 3: true}
	iterations = 0
	for range c {
		iterations++
		clear(c)
	}
	fmt.Println(iterations, len(c))

	g := map[int]int{0: 0}
	for k := range g {
		if k < 50 {
			g[k+100] = k
		}
	}
	fmt.Println(len(g), g[100])

	r := map[string]int{"x": 1}
	for k, v := range r {
		r = map[string]int{}
		r[k] = v + 1
	}
	fmt.Println(r)
}
