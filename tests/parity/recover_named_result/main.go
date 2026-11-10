package main

import "fmt"

func divide(a, b int) (q int) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r)
			q = -1
		}
	}()
	if b == 0 {
		panic("division by zero")
	}
	q = a / b
	return
}

func set() (n int) {
	defer func() {
		recover()
	}()
	defer func() {
		panic("late")
	}()
	return 5
}

func main() {
	fmt.Println(divide(7, 2))
	fmt.Println(divide(1, 0))
	fmt.Println("result kept after late panic:", set())
}
