package main

import (
	"fmt"
	"sync"
)

func main() {
	jobs := make(chan int, 3)
	jobs <- 1
	jobs <- 2
	jobs <- 3
	close(jobs)

	var wg sync.WaitGroup
	results := make(chan int, 3)
	for j := range jobs {
		wg.Add(1)
		go func() {
			defer wg.Done()
			results <- j * 10
		}()
	}
	wg.Wait()
	close(results)

	total := 0
	for r := range results {
		total += r
	}
	fmt.Println("total", total)

	var ptrs []*int
	nums := make(chan int, 2)
	nums <- 4
	nums <- 5
	close(nums)
	for n := range nums {
		ptrs = append(ptrs, &n)
	}
	fmt.Println(*ptrs[0], *ptrs[1])
}
