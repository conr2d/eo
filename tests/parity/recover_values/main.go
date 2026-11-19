package main

import (
	"errors"
	"fmt"
	"runtime"
)

type T struct {
	A int
}

type Loud struct {
	N int
}

func (l Loud) String() string {
	return fmt.Sprintf("loud-%d", l.N)
}

type Bad struct{}

func (Bad) String() string {
	panic(Loud{N: 7})
}

func classify(name string, f func()) {
	defer func() {
		r := recover()
		err, isErr := r.(error)
		_, isRuntime := r.(runtime.Error)
		fmt.Printf("%s: %T %v %t %t\n", name, r, r, isErr, isRuntime)
		if isErr {
			var re runtime.Error
			fmt.Println("  message:", err.Error(), errors.As(err, &re))
		}
	}()
	f()
}

func main() {
	var p *T
	s := []int{}
	var i any = "x"
	z := 0
	classify("nil", func() { fmt.Println(p.A) })
	classify("index", func() { fmt.Println(s[3]) })
	classify("slice", func() { fmt.Println(s[1:5]) })
	classify("assert", func() { fmt.Println(i.(int)) })
	classify("divide", func() { fmt.Println(1 / z) })
	classify("nil panic", func() { panic(nil) })
	classify("string", func() { panic("str") })
	classify("error", func() { panic(errors.New("boom")) })
	classify("wrapped", func() { panic(fmt.Errorf("ctx: %w", errors.New("inner"))) })
	classify("struct", func() { panic(T{A: 1}) })
	classify("stringer", func() { panic(Loud{N: 2}) })
	classify("repanic", func() {
		defer func() {
			panic(recover())
		}()
		panic(Loud{N: 3})
	})
	fmt.Println(Bad{})
	fmt.Printf("%s|%d\n", Bad{}, Bad{})
}
