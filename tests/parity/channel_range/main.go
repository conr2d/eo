package main

import "fmt"

func produce(n int) chan int {
	ch := make(chan int)
	go func() {
		for i := range n {
			ch <- i * i
		}
		close(ch)
	}()
	return ch
}

func main() {
	sum := 0
	for v := range produce(5) {
		if v == 4 {
			continue
		}
		sum += v
	}
	fmt.Println("sum", sum)

	count := 0
	for range produce(3) {
		count++
	}
	fmt.Println("count", count)

	words := make(chan string, 4)
	words <- "a"
	words <- "b"
	words <- "stop"
	words <- "c"
	close(words)
	for w := range words {
		if w == "stop" {
			break
		}
		fmt.Println("word", w)
	}
	fmt.Println("left", <-words)

	var last string
	for last = range words {
	}
	fmt.Println("last", last == "", len(last))
}
