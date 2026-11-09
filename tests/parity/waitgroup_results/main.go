package main

import (
	"fmt"
	"sync"
)

func main() {
	results := make(chan int, 5)
	var wg sync.WaitGroup

	for i := 1; i <= 5; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			results <- i * i
		}()
	}

	wg.Wait()
	close(results)

	sum := 0
	for j := 0; j < 5; j++ {
		sum += <-results
	}
	fmt.Println("sum", sum)
}
