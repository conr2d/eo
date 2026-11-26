package main

import (
	"fmt"
	"sync"
	"sync/atomic"
)

type node struct {
	name string
}

type stats struct {
	hits  atomic.Int64
	ready atomic.Bool
	head  atomic.Pointer[node]
}

func attempt(name string, f func()) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println(name, "panic:", r)
		}
	}()
	f()
}

func typed() {
	var i32 atomic.Int32
	var u32 atomic.Uint32
	var u64 atomic.Uint64
	var up atomic.Uintptr
	i32.Store(2147483647)
	fmt.Println(i32.Add(1), i32.Load())
	fmt.Println(i32.Swap(5), i32.CompareAndSwap(4, 9), i32.CompareAndSwap(5, 9), i32.Load())
	fmt.Println(i32.And(12), i32.Or(6), i32.Load())
	fmt.Println(u32.Add(^uint32(0)), u64.Add(1<<63), u64.Add(1<<63), up.Add(7))
	fmt.Println(u32.Or(0xf0), u32.And(0x3c), u32.Load(), u64.Swap(3), up.CompareAndSwap(7, 8), up.Load())

	var b atomic.Bool
	fmt.Println(b.Load(), b.Swap(true), b.CompareAndSwap(false, true), b.CompareAndSwap(true, false), b.Load())

	var p atomic.Pointer[node]
	a := &node{"a"}
	c := &node{"c"}
	fmt.Println(p.Load() == nil, p.CompareAndSwap(nil, a), p.Load().name)
	fmt.Println(p.CompareAndSwap(&node{"a"}, c), p.Swap(c).name, p.Load().name)
	p.Store(nil)
	fmt.Println(p.Load() == nil)
}

func functions() {
	var n int64
	var m uint32
	fmt.Println(atomic.AddInt64(&n, 5), atomic.AddInt64(&n, -7), atomic.LoadInt64(&n))
	atomic.StoreInt64(&n, 40)
	fmt.Println(atomic.SwapInt64(&n, 41), atomic.CompareAndSwapInt64(&n, 41, 42), n)
	fmt.Println(atomic.AddUint32(&m, ^uint32(0)), atomic.OrUint32(&m, 1), atomic.AndUint32(&m, 3), m)
	var i32 int32
	var u64 uint64
	var up uintptr
	fmt.Println(atomic.AddInt32(&i32, -3), atomic.OrInt32(&i32, 8), atomic.AndInt32(&i32, 14), atomic.SwapInt32(&i32, 1))
	fmt.Println(atomic.CompareAndSwapInt32(&i32, 2, 3), atomic.CompareAndSwapInt32(&i32, 1, 3), atomic.LoadInt32(&i32))
	atomic.StoreUint64(&u64, 1<<63)
	fmt.Println(atomic.AddUint64(&u64, 1<<63), atomic.OrUint64(&u64, 6), atomic.AndUint64(&u64, 3), atomic.SwapUint64(&u64, 9))
	fmt.Println(atomic.CompareAndSwapUint64(&u64, 9, 10), atomic.LoadUint64(&u64), atomic.AndInt64(&n, 3), atomic.OrInt64(&n, 4), n)
	atomic.StoreUintptr(&up, 5)
	fmt.Println(atomic.AddUintptr(&up, 2), atomic.SwapUintptr(&up, 3), atomic.CompareAndSwapUintptr(&up, 3, 4), atomic.LoadUintptr(&up))
	fmt.Println(atomic.OrUintptr(&up, 1), atomic.AndUintptr(&up, 1), atomic.LoadUint32(&m), atomic.SwapUint32(&m, 7), atomic.CompareAndSwapUint32(&m, 7, 8), m)
	atomic.StoreInt32(&i32, 6)
	atomic.StoreUint32(&m, 6)
	fmt.Println(i32, m)
	var none *int32
	attempt("AddInt32", func() { atomic.AddInt32(none, 1) })
}

func values() {
	var v atomic.Value
	fmt.Println(v.Load() == nil)
	attempt("Store nil", func() { v.Store(nil) })
	fmt.Println(v.CompareAndSwap(1, 2), v.CompareAndSwap(nil, 1), v.Load())
	v.Store(3)
	attempt("Store string", func() { v.Store("x") })
	attempt("Swap nil", func() { v.Swap(nil) })
	attempt("Swap string", func() { v.Swap("x") })
	fmt.Println(v.Swap(4), v.Load())
	fmt.Println(v.CompareAndSwap(3, 5), v.CompareAndSwap(4, 5), v.Load())
	attempt("CompareAndSwap nil", func() { v.CompareAndSwap(5, nil) })
	attempt("CompareAndSwap mixed", func() { v.CompareAndSwap("a", 6) })
	attempt("CompareAndSwap string", func() { v.CompareAndSwap(nil, "b") })

	var w atomic.Value
	w.Store(map[string]int{"a": 1})
	cur := w.Load().(map[string]int)
	fmt.Println(cur["a"], len(cur))
	attempt("CompareAndSwap map", func() { w.CompareAndSwap(map[string]int{}, map[string]int{}) })

	var s atomic.Value
	fmt.Println(s.Swap([]int{1}) == nil)
	fmt.Println(s.Load())
}

func goroutines() {
	s := new(stats)
	var count int64
	var wg sync.WaitGroup
	for g := 0; g < 8; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := 0; i < 1000; i++ {
				s.hits.Add(1)
				atomic.AddInt64(&count, 2)
			}
			s.head.Store(&node{"done"})
			s.ready.Store(true)
		}()
	}
	wg.Wait()
	fmt.Println(s.hits.Load(), atomic.LoadInt64(&count), s.ready.Load(), s.head.Load().name)
}

func main() {
	typed()
	functions()
	values()
	goroutines()
}
