package main

import (
	"errors"
	"fmt"
	"slices"
	"strings"
	"sync"
)

type Point struct {
	X, Y int
}

type Key struct {
	Name  string
	Value any
}

type Celsius float64

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

var errA = errors.New("a")

func interfaceKeys() {
	m := map[any]int{}
	m[1] = 1
	m["1"] = 2
	m[int8(1)] = 3
	m[Point{1, 2}] = 4
	m[[2]int{1, 2}] = 5
	m[true] = 6
	m[nil] = 7
	m[Celsius(1)] = 8
	fmt.Println(len(m), m[1], m["1"], m[int8(1)], m[Point{1, 2}], m[[2]int{1, 2}], m[true], m[nil], m[Celsius(1)], m[1.0])
	_, ok := m[int16(1)]
	fmt.Println(ok)
	delete(m, Point{1, 2})
	_, ok = m[Point{1, 2}]
	fmt.Println(len(m), ok)

	var zero float64
	negZero := -zero
	nan := zero / zero
	f := map[any]string{}
	f[0.0] = "zero"
	f[negZero] = "negative zero"
	f[nan] = "nan"
	f[nan] = "nan"
	fmt.Println(len(f), f[0.0], f)
	for k := range f {
		if k == 0.0 {
			fmt.Println(1/k.(float64) < 0)
		}
	}

	errs := map[error]int{errA: 1}
	errs[errA]++
	errs[errors.New("a")] = 5
	fmt.Println(len(errs), errs[errA])

	c1 := make(chan int)
	c2 := make(chan int)
	chans := map[chan int]string{c1: "one", c2: "two"}
	chans[c1] += "!"
	delete(chans, c2)
	fmt.Println(chans[c1], chans[c2], len(chans))

	keys := map[Key]int{{"a", 1}: 1, {"a", "1"}: 2, {"b", nil}: 3}
	keys[Key{"a", 1}] += 10
	fmt.Println(len(keys), keys[Key{"a", 1}], keys[Key{"b", nil}])

	words := map[any]int{"b": 2, "c": 3, "a": 1}
	nums := map[any]string{3: "c", 1: "a", 2: "b"}
	fmt.Println(words, nums)

	attempt("index slice", func() { _ = m[[]int{1}] })
	attempt("assign map", func() { m[map[string]int{}] = 1 })
	attempt("delete slice", func() { delete(m, []string{"x"}) })
	var nilMap map[any]int
	attempt("nil map index", func() { _ = nilMap[[]byte("x")] })
	attempt("nil map delete", func() { delete(nilMap, []int{}) })
	attempt("struct field", func() { keys[Key{"x", []int{}}] = 1 })
	attempt("nil map ok", func() { _, ok := nilMap[1]; fmt.Println(ok) })
}

func syncMap() {
	var m sync.Map
	v, ok := m.Load("missing")
	fmt.Println(v, ok)
	m.Store("a", 1)
	m.Store(2, "two")
	m.Store(Point{1, 2}, []int{1})
	v, ok = m.Load("a")
	fmt.Println(v, ok)
	actual, loaded := m.LoadOrStore("a", 100)
	fmt.Println(actual, loaded)
	actual, loaded = m.LoadOrStore("b", 200)
	fmt.Println(actual, loaded)
	previous, loaded := m.Swap("a", 10)
	fmt.Println(previous, loaded)
	previous, loaded = m.Swap("c", 30)
	fmt.Println(previous, loaded)
	fmt.Println(m.CompareAndSwap("a", 10, 11), m.CompareAndSwap("a", 10, 12), m.CompareAndSwap("zz", nil, 1))
	fmt.Println(m.CompareAndDelete("b", 1), m.CompareAndDelete("b", 200), m.CompareAndDelete("zz", nil))
	v, loaded = m.LoadAndDelete("c")
	fmt.Println(v, loaded)
	v, loaded = m.LoadAndDelete("c")
	fmt.Println(v, loaded)
	m.Delete(2)
	m.Delete("never")
	var entries []string
	m.Range(func(k, v any) bool {
		entries = append(entries, fmt.Sprint(k, "=", v))
		return true
	})
	slices.SortFunc(entries, strings.Compare)
	fmt.Println(entries)
	count := 0
	m.Store("d", 4)
	m.Range(func(k, v any) bool {
		count++
		m.Delete(k)
		return false
	})
	fmt.Println(count)
	m.Clear()
	m.Range(func(k, v any) bool {
		fmt.Println("after clear", k)
		return true
	})
	v, ok = m.Load("a")
	fmt.Println(v, ok)
	attempt("load slice", func() { m.Load([]int{1}) })
	attempt("store map", func() { m.Store(map[int]int{}, 1) })
	m.Store("s", []int{1})
	attempt("compare slice", func() { m.CompareAndSwap("s", []int{1}, 2) })
	attempt("compare delete slice", func() { m.CompareAndDelete("s", []int{1}) })
	fmt.Println(m.CompareAndSwap("s", 1, 2))
}

func syncMapGoroutines() {
	var m sync.Map
	var wg sync.WaitGroup
	for g := 0; g < 8; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := 0; i < 100; i++ {
				k := i % 25
				actual, _ := m.LoadOrStore(k, g*1000+i)
				if actual == nil {
					panic("nil")
				}
			}
		}()
	}
	wg.Wait()
	n := 0
	m.Range(func(k, v any) bool {
		n++
		return true
	})
	fmt.Println("keys", n)
}

type buffer struct {
	id int
	b  strings.Builder
}

func pool() {
	made := 0
	p := sync.Pool{New: func() any {
		made++
		return &buffer{id: made}
	}}
	a := p.Get().(*buffer)
	b := p.Get().(*buffer)
	fmt.Println(a.id, b.id, made)
	a.b.WriteString("x")
	p.Put(a)
	p.Put(nil)
	c := p.Get().(*buffer)
	fmt.Println(c.id == a.id, c.b.String(), made)
	var empty sync.Pool
	fmt.Println(empty.Get())
	empty.Put("x")
	fmt.Println(empty.Get(), empty.Get())
}

var counter int

func once() {
	var o sync.Once
	for i := 0; i < 3; i++ {
		o.Do(func() { counter++ })
	}
	fmt.Println(counter)
	var p sync.Once
	attempt("once panic", func() { p.Do(func() { panic("boom") }) })
	attempt("once after panic", func() {
		p.Do(func() { fmt.Println("not called") })
		fmt.Println("done")
	})
	var wg sync.WaitGroup
	var shared sync.Once
	calls := 0
	for g := 0; g < 8; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			shared.Do(func() { calls++ })
		}()
	}
	wg.Wait()
	fmt.Println(calls)
}

func onceFuncs() {
	n := 0
	f := sync.OnceFunc(func() { n++ })
	f()
	f()
	fmt.Println(n)
	g := sync.OnceFunc(func() { n++; panic(fmt.Sprint("panic ", n)) })
	attempt("first", g)
	attempt("second", g)
	fmt.Println(n)

	v := sync.OnceValue(func() int { n++; return n * 10 })
	fmt.Println(v(), v(), n)
	w := sync.OnceValue(func() string { panic(errA) })
	attempt("value first", func() { fmt.Println(w()) })
	attempt("value second", func() { fmt.Println(w()) })
	var target error
	func() {
		defer func() {
			target, _ = recover().(error)
		}()
		w()
	}()
	fmt.Println(target == errA)

	vs := sync.OnceValues(func() (int, error) { n++; return n, errA })
	x, err := vs()
	y, err2 := vs()
	fmt.Println(x, err, y, err2, n)
	zs := sync.OnceValues(func() ([]int, bool) { var s []int; s[1] = 0; return s, true })
	attempt("values first", func() { zs() })
	attempt("values second", func() { zs() })
}

func main() {
	interfaceKeys()
	syncMap()
	syncMapGoroutines()
	pool()
	once()
	onceFuncs()
}
