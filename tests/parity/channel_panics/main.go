package main

import "fmt"

func send(ch chan int) (r any) {
	defer func() {
		r = recover()
	}()
	ch <- 1
	return nil
}

func closeIt(ch chan int) (r any) {
	defer func() {
		r = recover()
	}()
	close(ch)
	return nil
}

func makeChan(n int) (r any) {
	defer func() {
		r = recover()
	}()
	fmt.Println(cap(make(chan int, n)))
	return nil
}

func main() {
	ch := make(chan int, 1)
	fmt.Println(closeIt(ch))
	r := send(ch)
	fmt.Printf("%T %v\n", r, r)
	fmt.Println(closeIt(ch))
	var nilCh chan int
	fmt.Println(closeIt(nilCh))
	fmt.Println(makeChan(-1), makeChan(2))
	_, isRuntime := r.(interface{ RuntimeError() })
	fmt.Println(isRuntime)
}
