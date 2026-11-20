package main

import "fmt"

type Counter struct {
	n int
}

func (c *Counter) Add(d int) int {
	c.n += d
	return c.n
}

func (c Counter) Get() int {
	return c.n
}

type Hook struct {
	name string
	on   func(string) string
}

func twice(n int) int {
	return n * 2
}

func apply(f func(int) int, v int) int {
	return f(v)
}

func makeAdder(k int) func(int) int {
	return func(n int) int { return n + k }
}

func compose(f, g func(int) int) func(int) int {
	return func(n int) int { return g(f(n)) }
}

func divmod(a, b int) (int, int) {
	return a / b, a % b
}

func split(f func(int, int) (int, int), a, b int) {
	q, r := f(a, b)
	fmt.Println("split", q, r)
}

func trace(label string, v int) int {
	fmt.Println("eval", label)
	return v
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	var f func(int) int
	fmt.Println("nil", f == nil, f != nil)
	f = twice
	fmt.Println("named", f(3), f == nil)
	fmt.Println("apply", apply(twice, 4), apply(makeAdder(10), 5), apply(f, 6))
	fmt.Println("chained", makeAdder(1)(2), compose(twice, makeAdder(1))(5))
	split(divmod, 17, 5)

	op := func(n int) int { return n - 1 }
	fmt.Println("op", op(10))
	op = twice
	fmt.Println("op", op(10))

	fs := []func(int) int{twice, makeAdder(3), nil}
	for i, g := range fs {
		if g == nil {
			fmt.Println("fs", i, "nil")
			continue
		}
		fmt.Println("fs", i, g(7))
	}

	table := map[string]func(int) int{"twice": twice}
	if g, ok := table["twice"]; ok {
		fmt.Println("table", g(8))
	}
	fmt.Println("table missing", table["missing"] == nil)

	h := Hook{name: "shout"}
	fmt.Println("hook", h.name, h.on == nil)
	h.on = func(s string) string { return s + "!" }
	fmt.Println("hook", h.on("hi"), h.on == nil)

	var fib func(int) int
	fib = func(n int) int {
		if n < 2 {
			return n
		}
		return fib(n-1) + fib(n-2)
	}
	fmt.Println("fib", fib(10))

	c := &Counter{}
	add := c.Add
	add(2)
	add(3)
	fmt.Println("method value", c.n)

	v := Counter{n: 1}
	get := v.Get
	v.n = 100
	fmt.Println("value receiver", get(), v.Get())

	var w Counter
	addW := w.Add
	addW(4)
	fmt.Println("addressable", w.n)

	getC := Counter.Get
	addP := (*Counter).Add
	getP := (*Counter).Get
	fmt.Println("method expr", getC(v), addP(c, 10), getP(c))

	attempt("nil call", func() {
		var g func(int) int
		g(trace("argument", 1))
	})
	attempt("nil field", func() {
		h.on = nil
		fmt.Println(h.on("unreachable"))
	})
	attempt("nil method value", func() {
		var np *Counter
		g := np.Get
		fmt.Println("unreachable", g == nil)
	})
	attempt("nil method expr", func() {
		fmt.Println(getP(nil))
	})
	attempt("nil receiver", func() {
		var np *Counter
		g := np.Add
		fmt.Println("bound", g == nil)
		g(1)
	})
	attempt("deferred nil", func() {
		var g func()
		defer g()
		fmt.Println("deferred nil registered")
	})
}
