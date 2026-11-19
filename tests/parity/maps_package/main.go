package main

import (
	"fmt"
	"maps"
)

func main() {
	src := map[string]int{"a": 1, "b": 2}
	dup := maps.Clone(src)
	dup["c"] = 3
	fmt.Println(src, dup)

	var none map[string]int
	fmt.Println(maps.Clone(none) == nil, maps.Equal(none, map[string]int{}))

	maps.Copy(dup, map[string]int{"a": 10, "d": 4})
	fmt.Println(dup)

	maps.DeleteFunc(dup, func(k string, v int) bool {
		return v%2 == 0
	})
	fmt.Println(dup)

	fmt.Println(maps.Equal(src, map[string]int{"b": 2, "a": 1}), maps.Equal(src, dup))
	labels := map[string]string{"a": "1", "b": "2"}
	fmt.Println(maps.EqualFunc(src, labels, func(v int, s string) bool {
		return fmt.Sprint(v) == s
	}))
}
