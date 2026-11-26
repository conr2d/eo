package main

import (
	"fmt"
	"unsafe"
)

func words(b []byte) []uint64 {
	return *(*[]uint64)(unsafe.Pointer(&b))
}

func orBytes(dst, a, b []byte) int {
	n := len(a)
	if len(b) < n {
		n = len(b)
	}
	w := n / 8
	if w > 0 {
		dw := *(*[]uint64)(unsafe.Pointer(&dst))
		aw := *(*[]uint64)(unsafe.Pointer(&a))
		bw := *(*[]uint64)(unsafe.Pointer(&b))
		for i := 0; i < w; i++ {
			dw[i] = aw[i] | bw[i]
		}
	}
	for i := n - n%8; i < n; i++ {
		dst[i] = a[i] | b[i]
	}
	return n
}

func main() {
	b := make([]byte, 24, 32)
	for i := range b {
		b[i] = byte(i + 1)
	}
	w := words(b)
	fmt.Println(len(w), cap(w))
	fmt.Printf("%#x %#x %#x\n", w[0], w[1], w[2])
	w[1] = 0
	fmt.Println(b)
	b[16] = 0xff
	fmt.Printf("%#x\n", w[2])

	tail := words(b[8:])
	fmt.Println(len(tail), cap(tail), tail[0] == w[1])

	var none []byte
	nw := words(none)
	fmt.Println(nw == nil, len(nw), cap(nw))
	empty := words([]byte{})
	fmt.Println(empty == nil, len(empty))

	u := []uint64{0x0102, 0x0304}
	back := *(*[]byte)(unsafe.Pointer(&u))
	fmt.Println(len(back), back[0], back[1])
	back[0] = 9
	fmt.Println(u)

	x := make([]byte, 19)
	y := make([]byte, 21)
	for i := range x {
		x[i] = byte(1 << (i % 8))
		y[i] = byte(i * 3)
	}
	dst := make([]byte, 19)
	fmt.Println(orBytes(dst, x, y), dst)
}
