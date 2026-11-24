package main

import (
	"fmt"
	"math/bits"
)

var state uint64 = 88172645463325252

func next() uint64 {
	state = state*6364136223846793005 + 1442695040888963407
	return state
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func counts(x uint64) {
	x32 := uint32(x)
	x16 := uint16(x)
	x8 := uint8(x)
	fmt.Println(bits.LeadingZeros64(x), bits.LeadingZeros32(x32), bits.LeadingZeros16(x16), bits.LeadingZeros8(x8), bits.LeadingZeros(uint(x)))
	fmt.Println(bits.TrailingZeros64(x), bits.TrailingZeros32(x32), bits.TrailingZeros16(x16), bits.TrailingZeros8(x8), bits.TrailingZeros(uint(x)))
	fmt.Println(bits.OnesCount64(x), bits.OnesCount32(x32), bits.OnesCount16(x16), bits.OnesCount8(x8), bits.OnesCount(uint(x)))
	fmt.Println(bits.Len64(x), bits.Len32(x32), bits.Len16(x16), bits.Len8(x8), bits.Len(uint(x)))
	fmt.Println(bits.Reverse64(x), bits.Reverse32(x32), bits.Reverse16(x16), bits.Reverse8(x8), bits.ReverseBytes64(x), bits.ReverseBytes32(x32), bits.ReverseBytes16(x16))
}

func rotations(x uint64, k int) {
	fmt.Println(bits.RotateLeft64(x, k), bits.RotateLeft32(uint32(x), k), bits.RotateLeft16(uint16(x), k), bits.RotateLeft8(uint8(x), k), bits.RotateLeft(uint(x), k))
}

func arithmetic(x, y uint64) {
	carry := y & 1
	sum, c := bits.Add64(x, y, carry)
	diff, b := bits.Sub64(x, y, carry)
	hi, lo := bits.Mul64(x, y)
	s32, c32 := bits.Add32(uint32(x), uint32(y), uint32(carry))
	d32, b32 := bits.Sub32(uint32(x), uint32(y), uint32(carry))
	h32, l32 := bits.Mul32(uint32(x), uint32(y))
	fmt.Println(sum, c, diff, b, hi, lo, s32, c32, d32, b32, h32, l32)
	if y != 0 {
		q, r := bits.Div64(x%y, lo, y)
		rem := bits.Rem64(x, lo, y)
		fmt.Println(q, r, rem)
	}
}

func main() {
	for _, x := range []uint64{0, 1, 0x80, 0xff00, 1 << 63, 18446744073709551615} {
		counts(x)
	}
	for i := 0; i < 100; i++ {
		x := next()
		y := next() >> (next() % 64)
		k := int(next()%130) - 65
		counts(x)
		rotations(x, k)
		arithmetic(x, y)
	}
	arithmetic(18446744073709551615, 18446744073709551615)
	arithmetic(0, 1)
	attempt("divide", func() { bits.Div64(1, 2, 0) })
	attempt("overflow", func() { bits.Div64(5, 0, 5) })
	attempt("div32", func() { bits.Div32(3, 0, 2) })
	attempt("rem", func() { bits.Rem64(1, 2, 0) })
	fmt.Println(bits.Rem64(5, 7, 5), bits.Rem32(9, 4, 7), bits.Rem(1, 0, 3), bits.UintSize)
	attempt("rem32", func() { bits.Rem32(1, 2, 0) })
}
