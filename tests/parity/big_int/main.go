package main

import (
	"fmt"
	"math/big"
)

type parseCase struct {
	s    string
	base int
}

type holder struct {
	N *big.Int
	M int
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func parse(c parseCase) {
	x, ok := new(big.Int).SetString(c.s, c.base)
	fmt.Println(c.s, c.base, x, ok)
}

func bits(i, j int64) {
	x := big.NewInt(i)
	y := big.NewInt(j)
	conj := new(big.Int).And(x, y)
	disj := new(big.Int).Or(x, y)
	excl := new(big.Int).Xor(x, y)
	diff := new(big.Int).AndNot(x, y)
	fmt.Printf("%v %v %v %v | ", conj, disj, excl, diff)
}

func main() {
	// Arithmetic returns its receiver, so calls chain, and the receiver may alias an operand.
	a := big.NewInt(1 << 62)
	b := new(big.Int).Mul(a, a)
	c := new(big.Int).Add(b, big.NewInt(-5))
	c.Sub(c, a).Mul(c, big.NewInt(3))
	n := new(big.Int).Neg(c)
	abs := new(big.Int).Abs(big.NewInt(-9))
	fmt.Println(a, b, c, c.Sign(), c.BitLen(), n, abs)
	var z big.Int
	z.SetUint64(1<<63 + 7)
	z.Add(&z, &z)
	nz := new(big.Int).Neg(&z)
	fmt.Println(&z, z.Cmp(a), a.Cmp(&z), a.Cmp(a), z.CmpAbs(nz), z.IsUint64(), z.IsInt64())
	var acc big.Int
	for i := int64(1); i <= 5; i++ {
		acc.Add(&acc, big.NewInt(i))
	}
	fmt.Println(acc.String())

	// Div and Mod are Euclidean, and Quo and Rem truncate.
	for _, p := range [][2]int64{{7, 3}, {-7, 3}, {7, -3}, {-7, -3}, {6, 3}, {-6, 3}} {
		x := big.NewInt(p[0])
		y := big.NewInt(p[1])
		d := new(big.Int).Div(x, y)
		md := new(big.Int).Mod(x, y)
		q, m := new(big.Int).DivMod(x, y, new(big.Int))
		qu := new(big.Int).Quo(x, y)
		re := new(big.Int).Rem(x, y)
		qt, r := new(big.Int).QuoRem(x, y, new(big.Int))
		fmt.Println(p[0], p[1], d, md, q, m, qu, re, qt, r)
	}

	// Bit operations on negative values act on two's complement.
	vals := []int64{12, -12, 5, -5, 0}
	for _, i := range vals {
		for _, j := range vals[:4] {
			bits(i, j)
		}
		x := big.NewInt(i)
		comp := new(big.Int).Not(x)
		lsh := new(big.Int).Lsh(x, 70)
		rsh := new(big.Int).Rsh(x, 2)
		rshAll := new(big.Int).Rsh(x, 100)
		fmt.Println(comp, lsh, rsh, rshAll, x.Bit(0), x.Bit(2), x.Bit(200), x.TrailingZeroBits())
	}
	s := new(big.Int).SetBit(big.NewInt(-12), 2, 0)
	t := new(big.Int).SetBit(big.NewInt(5), 130, 1)
	u := new(big.Int).SetBit(big.NewInt(-1), 3, 0)
	cleared := new(big.Int).SetBit(t, 130, 0)
	fmt.Println(s, t, u, cleared)

	// Conversions to and from integers, bytes, and words.
	m := new(big.Int).SetBytes([]byte{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02})
	zero := new(big.Int)
	fmt.Println(m, m.Bytes(), m.Uint64(), m.Int64(), m.Bits(), len(zero.Bytes()), zero.Bytes() == nil, zero.Bits() == nil)
	neg := new(big.Int).Lsh(big.NewInt(-1), 63)
	below := new(big.Int).Sub(neg, big.NewInt(1))
	minus1 := big.NewInt(-1)
	minus3 := big.NewInt(-3)
	fmt.Println(neg.Int64(), neg.IsInt64(), below.IsInt64(), minus1.Uint64(), minus3.IsUint64())
	buf := make([]byte, 12)
	fmt.Println(m.FillBytes(buf), buf)
	w := new(big.Int).SetBits([]big.Word{3, 0, 1, 0})
	empty := new(big.Int).SetBytes(nil)
	eight := new(big.Int).SetInt64(-8)
	fmt.Println(w, w.Bits(), len(w.Bits()), empty, eight)
	nib := 9
	words := make([]big.Word, 2)
	words[0] = 0xff
	words[0] <<= 4
	words[0] += big.Word(nib)
	words[1] = ^words[0]
	fromWords := new(big.Int).SetBits(words)
	fmt.Println(fromWords, words)

	// Exp, with and without a modulus, and ModInverse.
	two := big.NewInt(2)
	hundred := big.NewInt(100)
	prime := big.NewInt(1000007)
	three := big.NewInt(3)
	minusThree := big.NewInt(-3)
	seven := big.NewInt(7)
	minusSeven := big.NewInt(-7)
	e1 := new(big.Int).Exp(two, hundred, nil)
	e2 := new(big.Int).Exp(two, hundred, prime)
	e3 := new(big.Int).Exp(minusThree, three, nil)
	e4 := new(big.Int).Exp(minusThree, three, seven)
	e5 := new(big.Int).Exp(minusThree, three, minusSeven)
	fmt.Println(e1, e2, e3, e4, e5)
	e6 := new(big.Int).Exp(two, minus1, nil)
	e7 := new(big.Int).Exp(three, minus1, seven)
	e8 := new(big.Int).Exp(minusThree, minus1, seven)
	e9 := new(big.Int).Exp(two, minus1, big.NewInt(4))
	fmt.Println(e6, e7, e8, e9 == nil)
	one := big.NewInt(1)
	e10 := new(big.Int).Exp(big.NewInt(5), zero, one)
	e11 := new(big.Int).Exp(zero, zero, nil)
	e12 := new(big.Int).Exp(zero, three, big.NewInt(9))
	fmt.Println(e10, e11, e12)
	for _, p := range [][2]int64{{3, 7}, {12, 5}, {-3, 7}, {3, -7}, {2, 4}, {0, 1}, {0, 5}, {1, 0}, {4, 0}, {100, 1}, {7, 1000000007}} {
		g := big.NewInt(p[0])
		k := big.NewInt(p[1])
		inv := new(big.Int).ModInverse(g, k)
		fmt.Printf("%v ", inv)
	}
	fmt.Println()

	// SetString with bases, prefixes, and separators.
	for _, in := range []parseCase{{"123", 10}, {"-0x_ff", 0}, {"0b101", 0}, {"0o17", 0}, {"017", 0}, {"0", 0}, {"08", 0}, {"1_000", 0}, {"1__0", 0}, {"_1", 0}, {"1_", 0}, {"1_000", 10}, {"ff", 16}, {"FF", 16}, {"zZ", 62}, {"Zz", 36}, {"", 10}, {"-", 10}, {"+5", 10}, {"12a", 10}, {"0x", 0}, {"0_7", 0}, {"99999999999999999999999999", 10}} {
		parse(in)
	}

	// Text and printing: a *big.Int formats itself, and a nil one prints <nil>.
	h := new(big.Int).SetBytes([]byte{0xde, 0xad, 0xbe, 0xef})
	nh := new(big.Int).Neg(h)
	fmt.Println(h.Text(16), nh.Text(2), h.Text(62), h.String(), nh.String())
	five := big.NewInt(5)
	fmt.Printf("%v %d %s %x %X %b %o %O %q %c\n", nh, nh, nh, nh, nh, five, h, h, h, h)
	var nilInt *big.Int
	fmt.Println(nilInt, nilInt.String(), nilInt.Text(10))
	fmt.Printf("%v %d %x %q %T %T\n", nilInt, nilInt, nilInt, nilInt, nilInt, h)
	fmt.Println([]*big.Int{h, nil, minus1}, map[string]*big.Int{"k": h})
	fmt.Printf("%x %d\n", []*big.Int{h, nh}, holder{h, 3})
	ten := big.NewInt(10)
	fmt.Println(*ten, big.Int{}, *nh)

	// In an interface: a fmt.Stringer, compared by pointer.
	var x any = h
	sv, ok := x.(fmt.Stringer)
	same := new(big.Int).Set(h)
	fmt.Println(x, ok, sv.String(), x == any(h), x == any(same))
	if v, ok := x.(*big.Int); ok {
		fmt.Println(v.Uint64())
	}

	// Panics.
	attempt("div", func() { new(big.Int).Div(big.NewInt(1), new(big.Int)) })
	attempt("quo", func() { new(big.Int).Quo(big.NewInt(1), new(big.Int)) })
	attempt("modinverse", func() { new(big.Int).ModInverse(big.NewInt(-1), new(big.Int)) })
	small := make([]byte, 1)
	attempt("fill", func() { big.NewInt(256).FillBytes(small) })
	attempt("bit", func() { big.NewInt(1).Bit(-1) })
	attempt("setbit", func() { new(big.Int).SetBit(big.NewInt(1), 1, 2) })
	attempt("setbit neg", func() { new(big.Int).SetBit(big.NewInt(-1), 1, 2) })
	attempt("text", func() { big.NewInt(1).Text(63) })
	attempt("base", func() { new(big.Int).SetString("1", 1) })
	attempt("nil", func() { fmt.Println(nilInt.Sign()) })
	attempt("nil receiver", func() { nilInt.Add(h, h) })
}
