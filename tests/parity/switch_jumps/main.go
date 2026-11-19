package main

import "fmt"

func grade(n int) {
	switch {
	case n >= 90:
		fmt.Println(n, "A")
		fallthrough
	case n >= 80:
		fmt.Println(n, "at least B")
		fallthrough
	case n >= 70:
		fmt.Println(n, "at least C")
	default:
		fmt.Println(n, "below C")
	}
}

func middle(n int) {
	switch n {
	case 1:
		fmt.Println(n, "one")
		fallthrough
	default:
		fmt.Println(n, "default")
		fallthrough
	case 2:
		fmt.Println(n, "two")
	case 3:
		fmt.Println(n, "three")
	}
}

func last(s string) {
	switch s {
	case "heap":
		fmt.Println("collect")
		fallthrough
	default:
		fmt.Println("profile", s)
	}
}

func find(xs []int, want int) {
	for i, x := range xs {
		switch {
		case x == want:
			fmt.Println("found", want, "at", i)
			break
		case x < 0:
			fmt.Println("negative at", i)
			if x < -5 {
				break
			}
			fmt.Println("small negative")
		default:
			continue
		}
		fmt.Println("after switch", i)
	}
}

func kinds(vs []any) {
	for _, v := range vs {
		switch x := v.(type) {
		case int:
			if x == 0 {
				break
			}
			fmt.Println("int", x)
		case string:
			fmt.Println("string", x)
		}
		fmt.Println("next")
	}
}

func inner() {
	switch n := 3; n {
	case 3:
		for i := 0; i < 5; i++ {
			if i == 2 {
				break
			}
			fmt.Println("loop", i)
		}
		fmt.Println("after loop")
		break
	}
	fmt.Println("after switch")
}

func main() {
	for _, n := range []int{95, 85, 75, 65} {
		grade(n)
	}
	for _, n := range []int{1, 2, 3, 4} {
		middle(n)
	}
	last("heap")
	last("cpu")
	find([]int{4, -1, -9, 7, 3}, 7)
	kinds([]any{1, 0, "s", 2.5})
	inner()
}
