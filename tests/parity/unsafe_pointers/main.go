package main

import (
	"fmt"
	"unsafe"
)

type sponge struct {
	a    [16]byte
	n    int
	rate int
}

type words [2]uint64

func lanes(s *sponge) *[2]uint64 {
	return (*[2]uint64)(unsafe.Pointer(&s.a))
}

func main() {
	s := &sponge{rate: 8}
	for i := range s.a {
		s.a[i] = byte(i + 1)
	}
	a := lanes(s)
	fmt.Printf("%#x %#x\n", a[0], a[1])
	a[1] ^= 0xff00000000000000
	fmt.Println(s.a, s.n, s.rate)

	half := (*[8]byte)(unsafe.Pointer(&s.a))
	half[0] = 0xee
	fmt.Println(*half, a[0]&0xff)

	w := (*words)(unsafe.Pointer(a))
	w[0] = 0
	fmt.Println(s.a[:8], w[1] == a[1])

	f := 1.5
	bits := *(*uint64)(unsafe.Pointer(&f))
	fmt.Printf("%#x\n", bits)

	var none *sponge
	fmt.Println((*[2]uint64)(unsafe.Pointer(none)) == nil)
	count := 0
	for range none.a {
		count++
	}
	fmt.Println(count, len(none.a))
}
