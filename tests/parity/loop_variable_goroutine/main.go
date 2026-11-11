package main

import "fmt"

func main() {
	start := make(chan bool)
	results := make(chan int)
	for i := 0; i < 4; i++ {
		go func() {
			<-start
			i *= 10
			results <- i
		}()
	}
	close(start)
	sum := 0
	for k := 0; k < 4; k++ {
		sum += <-results
	}
	fmt.Println("sum", sum)
}
