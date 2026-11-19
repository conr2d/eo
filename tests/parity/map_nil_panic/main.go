package main

import "fmt"

func write(m map[string]int) (r any) {
	defer func() {
		r = recover()
	}()
	m["a"] = 1
	return nil
}

func increment(m map[string]int) (r any) {
	defer func() {
		r = recover()
	}()
	m["a"]++
	return nil
}

func main() {
	var m map[string]int
	r := write(m)
	err, isErr := r.(error)
	fmt.Println(r, isErr)
	fmt.Printf("%T %v\n", r, err.Error())
	fmt.Println(increment(m))
	fmt.Println(write(map[string]int{}), len(m))
	m["b"] = 2
	fmt.Println("unreachable")
}
