package main

import (
	"crypto/subtle"
	"fmt"
	"hash"
	"io"
)

// fnv is FNV-1a over 64 bits, a hash.Hash64 and a hash.Cloner.
type fnv struct {
	sum uint64
}

func (f *fnv) Write(p []byte) (int, error) {
	for _, c := range p {
		f.sum ^= uint64(c)
		f.sum *= 1099511628211
	}
	return len(p), nil
}

func (f *fnv) Sum(b []byte) []byte {
	s := f.sum
	return append(b, byte(s>>56), byte(s>>48), byte(s>>40), byte(s>>32), byte(s>>24), byte(s>>16), byte(s>>8), byte(s))
}

func (f *fnv) Reset()         { f.sum = 14695981039346656037 }
func (f *fnv) Size() int      { return 8 }
func (f *fnv) BlockSize() int { return 1 }
func (f *fnv) Sum64() uint64  { return f.sum }
func (f *fnv) Clone() (hash.Cloner, error) {
	c := *f
	return &c, nil
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func digest(h hash.Hash, s string) {
	io.WriteString(h, s)
	fmt.Printf("%x %d %d\n", h.Sum(nil), h.Size(), h.BlockSize())
}

func main() {
	a := []byte{1, 2, 3}
	b := []byte{1, 2, 4}
	fmt.Println(subtle.ConstantTimeCompare(a, a), subtle.ConstantTimeCompare(a, b), subtle.ConstantTimeCompare(a, a[:2]), subtle.ConstantTimeCompare(nil, []byte{}))
	fmt.Println(subtle.ConstantTimeSelect(1, 7, 9), subtle.ConstantTimeSelect(0, 7, 9), subtle.ConstantTimeSelect(5, 7, 9))
	fmt.Println(subtle.ConstantTimeByteEq(3, 3), subtle.ConstantTimeByteEq(3, 4), subtle.ConstantTimeEq(-1, -1), subtle.ConstantTimeEq(-1, 1))
	fmt.Println(subtle.ConstantTimeLessOrEq(2, 3), subtle.ConstantTimeLessOrEq(3, 3), subtle.ConstantTimeLessOrEq(4, 3), subtle.ConstantTimeLessOrEq(-5, 3))
	x := []byte{1, 2, 3}
	subtle.ConstantTimeCopy(0, x, []byte{7, 8, 9})
	fmt.Println(x)
	subtle.ConstantTimeCopy(1, x, []byte{7, 8, 9})
	fmt.Println(x)
	attempt("copy", func() { subtle.ConstantTimeCopy(1, x, a[:2]) })

	dst := make([]byte, 5)
	n := subtle.XORBytes(dst, []byte{0xf0, 0x0f, 0xff, 0x00}, []byte{0xff, 0xff, 0x0f})
	fmt.Println(n, dst)
	fmt.Println(subtle.XORBytes(nil, nil, a))
	buf := []byte{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
	n = subtle.XORBytes(buf[:6], buf[:6], buf[6:])
	fmt.Println(n, buf)
	attempt("short", func() { subtle.XORBytes(dst[:1], a, b) })
	attempt("overlap", func() { subtle.XORBytes(buf[1:7], buf[:6], buf[:6]) })
	ran := false
	subtle.WithDataIndependentTiming(func() { ran = true })
	fmt.Println(ran)

	f := &fnv{}
	f.Reset()
	var h hash.Hash = f
	digest(h, "hello")
	var h64 hash.Hash64 = f
	fmt.Println(h64.Sum64())
	var c hash.Cloner = f
	d, err := c.Clone()
	io.WriteString(d, " world")
	fmt.Printf("%x %x %v\n", f.Sum(nil), d.Sum([]byte{0xaa}), err)
	h.Reset()
	digest(h, "")
	var w io.Writer = h
	_, isHash := w.(hash.Hash64)
	_, isXOF := w.(hash.XOF)
	fmt.Println(isHash, isXOF)
}
