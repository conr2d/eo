package main

import "fmt"

func push(ch chan int, v int) int {
	ch <- v
	return v
}

func pick(name string, ch chan int) chan int {
	fmt.Println("eval", name)
	return ch
}

func main() {
	// A send that blocked on the full channel would deadlock, so each output line
	// also shows that the receive ran before the next call.
	ch := make(chan int, 1)
	ch <- 1
	fmt.Println(<-ch, push(ch, 2), <-ch)

	ch <- 3
	x := <-ch + push(ch, 4)*10
	fmt.Println(x, <-ch)

	// The channel operand of a send is evaluated before the value.
	other := make(chan int, 1)
	pick("chan", ch) <- push(pick("value", other), 5)+1
	fmt.Println(<-ch, <-other)

	// An element of a slice of channels next to a call that grows the slice.
	chs := []chan int{make(chan int, 1)}
	grow := func() int {
		chs = append(chs, make(chan int, 1))
		return 7
	}
	chs[0] <- grow()
	fmt.Println(<-chs[0], len(chs))

	// Receives in conditions.
	ch <- 8
	for n := 0; <-ch > push(ch, n); n++ {
		fmt.Println("loop", n)
	}
	if v := <-ch + push(ch, 2); v != 3 {
		fmt.Println("never")
	} else if <-pick("elif", ch) == push(ch, 9) {
		fmt.Println("never")
	} else {
		fmt.Println("else", <-ch)
	}
	ok := push(ch, 0) == 0 && <-pick("and", ch) == push(ch, 6)
	fmt.Println(ok, <-ch)
}
