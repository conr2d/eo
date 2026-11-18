package main

import (
	"fmt"
	"sync"
)

func main() {
	// Each iteration has its own range variables, so every pointer keeps its own value.
	var values []*int
	for _, v := range []int{1, 2, 3} {
		values = append(values, &v)
	}
	var indexes []*int
	for i := range 3 {
		indexes = append(indexes, &i)
	}
	var runes []*rune
	for _, r := range "ab" {
		runes = append(runes, &r)
	}
	for k := range values {
		fmt.Println(*values[k], *indexes[k])
	}
	fmt.Println(*runes[0], *runes[1])

	// A goroutine keeps its iteration's variables after the loop moves on.
	var wg sync.WaitGroup
	out := make([]string, 3)
	for i, w := range []string{"x", "y", "z"} {
		wg.Add(1)
		go func() {
			defer wg.Done()
			w += w
			out[i] = w
		}()
	}
	wg.Wait()
	fmt.Println(out)

	// A write through a pointer to the variable does not change the next iteration's value.
	for i := range 3 {
		p := &i
		*p += 10
		values[0] = p
		fmt.Println("iteration", *p)
	}
	fmt.Println(*values[0])

	// The assignment form writes an existing variable on each iteration.
	last := -1
	q := &last
	for last = range 4 {
		fmt.Println("last", *q)
	}
	fmt.Println(*q)
}
