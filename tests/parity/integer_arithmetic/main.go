package main

import "fmt"

func half(n int64) int64 {
	return n / 2
}

func main() {
	// Signed arithmetic wraps.
	big := int64(9223372036854775807)
	fmt.Println(big+1, -(big + 1), big*2)
	var i32 int32 = 2147483647
	i32++
	fmt.Println(i32)

	// Narrow types compute in their own width.
	var a int8 = 100
	var b int8 = 100
	c := a + b
	fmt.Println(c, (a+b)/2, -a, ^a, a*3)
	var u8 uint8 = 3
	fmt.Println(u8-5, ^u8, u8<<7, u8&^1)
	var u16 uint16 = 65535
	u16 *= u16
	fmt.Println(u16)
	s := "7x"
	d := s[0] - '0'
	fmt.Println(d, s[1]+200)

	// Division truncates toward zero; the most negative value divided by -1 wraps.
	n := int64(-7)
	m := int64(2)
	fmt.Println(n/m, n%m, n/2, n%2, half(n))
	minInt := -big - 1
	neg := int64(-1)
	fmt.Println(minInt/neg, minInt%neg)
	q := int64(100)
	q /= m
	q %= 7
	fmt.Println(q)

	// Shifts by a count of at least the width are defined.
	var k uint = 70
	x := int64(-5)
	fmt.Println(x<<k, x>>k, uint64(5)>>k, x>>1, x<<62, 1<<k)
	sh := 3
	one := int8(1)
	fmt.Println(1<<sh, one<<7, a>>sh)
	r := uint32(1)
	r <<= 31
	r >>= sh
	fmt.Println(r)

	// Precedence: shifts and bitwise operators bind like * and +.
	p := int64(3)
	fmt.Println(p+m<<2, p&1 == 1, p|m^1, p&^1+1)

	// Constants are exact.
	fmt.Println(1<<2+1, 7/2*2.0, 1<<62, 'a', 2.5)
	e := 10
	f := 1.5
	g := 'b' - 'a'
	fmt.Println(e*2, f*2, g, e+1<<20)
	var mask uint32 = 0xffffffff
	fmt.Println(mask&0xf0f0f0f0, mask+1)

	// Conversions.
	w := int64(300)
	m1 := int8(-1)
	u32 := uint32(4294967295)
	fmt.Println(int8(w), uint8(w), uint64(m1), int32(u32))
	fmt.Println(float64(w)/7, int64(2.9e3), int64(-2.9*f), float32(0.1), float64(float32(0.1)))
	var z float64
	fmt.Println(1/z, -1/z, z/z == z/z, 1e6, 1e-5, 123456.0)
}
