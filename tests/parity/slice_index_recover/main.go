package main

import "fmt"

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, recover())
	}()
	f()
}

func main() {
	s := []int{1, 2, 3}
	var none []int
	i, j, k := 5, -1, 2
	attempt("index", func() { fmt.Println(s[i]) })
	attempt("negative", func() { fmt.Println(s[j]) })
	attempt("nil", func() { fmt.Println(none[0]) })
	attempt("assign", func() { s[i] = 4 })
	attempt("high", func() { fmt.Println(s[:i]) })
	attempt("low", func() { fmt.Println(s[i:]) })
	attempt("inverted", func() { fmt.Println(s[k : k-1]) })
	attempt("max", func() { fmt.Println(s[0:1:i]) })
	attempt("highAboveMax", func() { fmt.Println(s[0:3:k]) })
	attempt("lowAboveHigh", func() { fmt.Println(s[k:1:3]) })
	attempt("negativeLow3", func() { fmt.Println(s[j:1:3]) })
	attempt("negativeMax", func() { fmt.Println(s[0:1:j]) })
	attempt("makeLen", func() { fmt.Println(make([]int, j)) })
	attempt("makeCap", func() { fmt.Println(make([]int, k, k-1)) })
	attempt("ok", func() { fmt.Println(s[1:3], s[2], s[:0], s[3:], len(s[1:1:3])) })
}
