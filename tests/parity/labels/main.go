package main

import "fmt"

func f() int {
	fmt.Println("f")
	return 1
}

func lo() int { return 1 }
func hi() int { return 3 }

// grid breaks and continues an outer three-clause loop whose variable escapes.
func grid() {
	var ps []*int
outer:
	for i := 0; i < 4; i++ {
		ps = append(ps, &i)
		for j := 0; j < 3; j++ {
			if i == 3 {
				break outer
			}
			if j > i {
				continue outer
			}
			fmt.Println("cell", i, j)
		}
		fmt.Println("row done", i)
	}
	for _, p := range ps {
		fmt.Println("p", *p)
	}
}

// rows continues a range loop from a switch in a nested loop, over a later declaration.
func rows() {
rows:
	for r, row := range [][]int{{1, 2}, {3, -1, 5}, {6}} {
		total := 0
		for _, v := range row {
			switch {
			case v < 0:
				fmt.Println("row", r, "negative")
				continue rows
			}
			total += v
		}
		avg := total / len(row)
		fmt.Println("row", r, total, avg)
	}
}

// items leaves a range loop from a type switch, next to an unlabeled break.
func items() {
items:
	for _, x := range []any{1, "two", 3, 4.5, nil, 5} {
		switch v := x.(type) {
		case nil:
			break items
		case string:
			fmt.Println("string", v)
			continue items
		case int:
			if v > 2 {
				break
			}
			fmt.Println("small int", v)
		}
		fmt.Println("after", x)
	}
}

// bound leaves a switch from a loop inside one of its clauses.
func bound(n int) {
sw:
	switch {
	case n > 5:
		for k := 0; ; k++ {
			if k*k > n {
				fmt.Println("root bound", k)
				break sw
			}
		}
	default:
		fmt.Println("small", n)
	}
	fmt.Println("bound done")
}

// window continues a loop whose condition needs temporaries.
func window() {
scan:
	for i := 0; i < lo()+hi(); i++ {
		for j := 0; j < 2; j++ {
			if i == 1 {
				continue scan
			}
			fmt.Println("scan", i, j)
		}
	}
}

// pump leaves and continues a loop from the clauses of a select.
func pump() {
	ch := make(chan int)
	done := make(chan bool)
	go func() {
		for i := 0; i < 5; i++ {
			ch <- i
		}
		close(done)
	}()
loop:
	for {
		select {
		case v := <-ch:
			if v%2 == 1 {
				continue loop
			}
			fmt.Println("even", v)
		case <-done:
			break loop
		}
	}
	fmt.Println("pumped")
}

// drain leaves a range loop over a channel from an inner loop.
func drain() {
	c := make(chan int, 5)
	for i := 0; i < 5; i++ {
		c <- i * 10
	}
	close(c)
recv:
	for v := range c {
		for {
			if v >= 30 {
				break recv
			}
			fmt.Println("got", v)
			break
		}
	}
	rest, ok := <-c
	fmt.Println("left", rest, ok)
}

// countdown loops with a backward goto.
func countdown(n int) {
again:
	if n > 0 {
		fmt.Println("tick", n)
		n--
		goto again
	}
}

// skip jumps forward over a statement with temporaries and a constant declaration.
func skip(n int) string {
	if n > 1 {
		goto done
	}
	fmt.Println("small", f()+f())
	const greeting = "hello"
	n = len(greeting)
done:
	return fmt.Sprint(n, " ", greeting)
}

// collect jumps back over a variable that escapes, so each pass makes a new one.
func collect() {
	var ps []*int
	n := 0
loop:
	v := n * n
	ps = append(ps, &v)
	n++
	if n < 3 {
		goto loop
	}
	for _, p := range ps {
		fmt.Println("p", *p)
	}
}

// guarded jumps forward over a defer statement.
func guarded(n int) (r int) {
	if n < 0 {
		goto fail
	}
	defer func() {
		if e := recover(); e != nil {
			fmt.Println("recovered", e)
			r = -1
		}
	}()
	r = 100 / n
	return
fail:
	fmt.Println("negative")
	return -2
}

// early can panic before its defer statement, which a goto jumps over.
func early(xs []int, i int) (r int) {
	if xs[i] < 0 {
		goto neg
	}
	defer func() { r++ }()
	return xs[i]
neg:
	return 0
}

func safeEarly(i int) {
	defer func() { fmt.Println("recovered", recover()) }()
	fmt.Println("early", early([]int{1, -2}, i))
}

func main() {
	grid()
	rows()
	items()
	bound(30)
	bound(3)
	window()
	pump()
	drain()
	countdown(3)
	fmt.Println(skip(5))
	fmt.Println(skip(0))
	collect()
	fmt.Println(guarded(4), guarded(0), guarded(-1))
	safeEarly(0)
	safeEarly(1)
	safeEarly(5)
	sum := func(xs []int) (t int) {
		i := 0
	next:
		if i < len(xs) {
			t += xs[i]
			i++
			goto next
		}
		return
	}
	fmt.Println(sum([]int{1, 2, 3, 4}))
}
