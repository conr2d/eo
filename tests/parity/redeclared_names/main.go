package main

import (
	"fmt"
	"sync"
)

func g() int {
	return 3
}

func main() {
	// A declaration that reads the variable it hides, at any depth.
	x := 1
	if x > 0 {
		x := x + 1
		{
			x := x * 10
			fmt.Println("nested", x)
		}
		fmt.Println("inner", x)
	}
	fmt.Println("outer", x)

	// The branches of an if whose init statement declares the name.
	if v := g(); v > 5 {
		fmt.Println("never", v)
	} else {
		v := v - 1
		fmt.Println("else", v)
	}

	// A range body redeclares a clause variable in a nested block.
	for i, v := range []int{4, 5} {
		if i == 1 {
			v := v * 100
			fmt.Println("range nested", v)
		}
		fmt.Println("range", v)
	}

	// The pre-1.22 idiom: each goroutine gets its own copy.
	var wg sync.WaitGroup
	out := make([]int, 3)
	for i := range 3 {
		i := i
		wg.Add(1)
		go func() {
			defer wg.Done()
			out[i] = i * i
		}()
	}
	wg.Wait()
	fmt.Println(out)

	// A boxed redeclaration, and a redeclaration of a boxed clause variable.
	var ptrs []*int
	for _, v := range []int{7, 8} {
		v := v + 1
		ptrs = append(ptrs, &v)
	}
	for _, v := range []int{20, 30} {
		ptrs = append(ptrs, &v)
		v := v + 1
		fmt.Println("copy", v)
	}
	for _, p := range ptrs {
		fmt.Println(*p)
	}
}
