package main

import (
	"fmt"
	"math/big"
)

// A linear congruential generator, so that both programs draw the same values.
var state uint64 = 88172645463325252

func next() uint64 {
	state = state*6364136223846793005 + 1442695040888963407
	return state
}

// draw returns a random Int of up to 40 bytes, of either sign.
func draw() *big.Int {
	n := int(next() % 41)
	buf := make([]byte, n)
	for i := range buf {
		buf[i] = byte(next() >> 56)
	}
	x := new(big.Int).SetBytes(buf)
	if next()&1 == 1 {
		x.Neg(x)
	}
	return x
}

func arithmetic(x, y *big.Int) {
	sum := new(big.Int).Add(x, y)
	difference := new(big.Int).Sub(x, y)
	product := new(big.Int).Mul(x, y)
	fmt.Printf("%x %x %x", sum, difference, product)
	if y.Sign() != 0 {
		q, r := new(big.Int).QuoRem(x, y, new(big.Int))
		d, m := new(big.Int).DivMod(x, y, new(big.Int))
		fmt.Printf(" %x %x %x %x", q, r, d, m)
	}
	fmt.Println()
}

func bitwise(x, y *big.Int, k uint) {
	conj := new(big.Int).And(x, y)
	disj := new(big.Int).Or(x, y)
	excl := new(big.Int).Xor(x, y)
	diff := new(big.Int).AndNot(x, y)
	comp := new(big.Int).Not(x)
	lsh := new(big.Int).Lsh(x, k)
	rsh := new(big.Int).Rsh(x, k)
	set := new(big.Int).SetBit(x, int(k), 1)
	unset := new(big.Int).SetBit(x, int(k), 0)
	bit := x.Bit(int(k))
	fmt.Printf("%x %x %x %x %x %x %x %x %x %d\n", conj, disj, excl, diff, comp, lsh, rsh, set, unset, bit)
}

func modular(x, y, m *big.Int) {
	e := new(big.Int).Exp(x, y, m)
	inv := new(big.Int).ModInverse(x, m)
	fmt.Printf("%x %x\n", e, inv)
}

func conversions(x *big.Int, base int) {
	text := x.Text(base)
	back, ok := new(big.Int).SetString(text, base)
	cmp := back.Cmp(x)
	bytes := x.Bytes()
	cmpAbs := x.CmpAbs(back)
	fmt.Println(text, ok, cmp, cmpAbs, len(bytes), x.BitLen(), x.TrailingZeroBits(), x.Int64(), x.Uint64(), x.IsInt64(), x.IsUint64(), len(x.Bits()))
}

func main() {
	for i := 0; i < 300; i++ {
		x := draw()
		y := draw()
		k := uint(next() % 140)
		arithmetic(x, y)
		bitwise(x, y, k)
		m := draw()
		if m.Sign() != 0 {
			small := big.NewInt(int64(next() % 50))
			modular(x, small, m)
		}
		conversions(x, int(2+next()%61))
	}
}
