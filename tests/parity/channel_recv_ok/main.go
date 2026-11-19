package main

import "fmt"

func main() {
	ch := make(chan int, 3)
	ch <- 7
	ch <- 0
	close(ch)
	fmt.Println(cap(ch), ch == nil)

	v, ok := <-ch
	fmt.Println(v, ok)
	v, ok = <-ch
	fmt.Println(v, ok)
	v, ok = <-ch
	fmt.Println(v, ok)
	if _, ok := <-ch; !ok {
		fmt.Println("closed")
	}
	<-ch

	done := make(chan struct{})
	results := make(chan string)
	go func() {
		for i := 0; i < 3; i++ {
			s, ok := <-results
			fmt.Println("got", s, ok)
		}
		close(done)
	}()
	results <- "x"
	results <- "y"
	close(results)
	<-done
	_, ok = <-done
	fmt.Println("done", ok)

	var nilCh chan int
	fmt.Println(nilCh == nil, cap(nilCh))
}
