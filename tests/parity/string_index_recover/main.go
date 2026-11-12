package main

import "fmt"

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, recover())
	}()
	f()
}

func main() {
	s := "abc"
	i, j := 5, -1
	var u uint = 1 << 63
	attempt("index", func() { fmt.Println(s[i]) })
	attempt("negative", func() { fmt.Println(s[j]) })
	attempt("high", func() { fmt.Println(s[:i]) })
	attempt("low", func() { fmt.Println(s[i:]) })
	attempt("inverted", func() { fmt.Println(s[2:1+j+1]) })
	attempt("unsigned", func() { fmt.Println(s[u]) })
	attempt("negativeLow", func() { fmt.Println(s[j:]) })
	attempt("negativeHigh", func() { fmt.Println(s[:j]) })
	attempt("highFirst", func() { fmt.Println(s[i:i+1]) })
	attempt("ok", func() { fmt.Println(s[1:3], s[2]) })
}
