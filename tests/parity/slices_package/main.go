package main

import (
	"fmt"
	"slices"
)

type Names []string

func deleteAt(s []int, i, j int) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("recovered:", r)
		}
	}()
	fmt.Println(slices.Delete(s, i, j))
}

func main() {
	var none []int
	fmt.Println(slices.Clone(none) == nil, slices.Clone([]int{}) == nil, len(slices.Clone([]int{})))

	src := []int{1, 2, 3}
	dup := slices.Clone(src)
	dup[0] = 10
	fmt.Println(src, dup)

	// Delete shifts in place and zeroes the tail, which the original slice still shows.
	s := []int{0, 1, 2, 3, 4, 5}
	d := slices.Delete(s, 1, 3)
	fmt.Println(d, len(d), s)
	fmt.Println(slices.Delete(s, 2, 2), len(slices.Delete(s, 6, 6)))
	deleteAt([]int{1, 2, 3}, 2, 1)
	deleteAt([]int{1, 2, 3}, 1, 4)
	deleteAt([]int{1, 2, 3}, -1, 1)

	// DeleteFunc calls del once per element, in order.
	words := []string{"a", "bb", "c", "dd", "e"}
	kept := slices.DeleteFunc(words, func(w string) bool {
		fmt.Printf("%s ", w)
		return len(w) == 2
	})
	fmt.Println()
	fmt.Printf("%q %q\n", kept, words)
	fmt.Println(slices.DeleteFunc(none, func(int) bool { return true }) == nil)

	fmt.Println(slices.Index(src, 2), slices.Index(src, 9), slices.Contains(src, 3), slices.Contains(none, 0))
	fmt.Println(slices.IndexFunc(src, func(v int) bool { return v > 1 }),
		slices.ContainsFunc(src, func(v int) bool { return v > 5 }))

	nums := []int{5, 2, 8, 1, 9}
	slices.SortFunc(nums, func(a, b int) int { return b - a })
	fmt.Println(nums)

	// SortStableFunc keeps equal elements in their order.
	people := []string{"bob:3", "al:1", "cy:3", "di:1", "ed:2"}
	slices.SortStableFunc(people, func(a, b string) int {
		return int(a[len(a)-1]) - int(b[len(b)-1])
	})
	fmt.Println(people)

	// The result of a function is converted back to a defined slice type.
	names := Names{"x", "y", "z"}
	names = slices.Delete(slices.Clone(names), 0, 1)
	fmt.Printf("%T %v\n", names, names)
}
