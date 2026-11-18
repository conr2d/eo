package main

import (
	"fmt"
	"sync"
)

func second() *int {
	s := []int{1, 2, 3}
	return &s[1]
}

func main() {
	p := second()
	*p += 40
	fmt.Println(*p)

	s := make([]int, 3)
	q := &s[2]
	*q = 5
	fmt.Println(s)
	s = append(s, 4)
	*q = 7
	fmt.Println(s, *q)

	results := make([]int, 4)
	var wg sync.WaitGroup
	for i := 0; i < 4; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			results[i] = i * i
		}()
	}
	wg.Wait()
	fmt.Println(results)
}
