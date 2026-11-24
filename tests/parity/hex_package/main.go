package main

import (
	"encoding/hex"
	"errors"
	"fmt"
)

func decode(s string) {
	b, err := hex.DecodeString(s)
	var ibe hex.InvalidByteError
	isInvalid := errors.As(err, &ibe)
	fmt.Println(s, b, len(b), err, err == hex.ErrLength, isInvalid, byte(ibe))
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	src := []byte{0x00, 0x01, 0xab, 0xff, 'h', 'i'}
	s := hex.EncodeToString(src)
	fmt.Println(s, hex.EncodedLen(len(src)), hex.DecodedLen(len(s)), hex.EncodeToString(nil) == "")
	dst := make([]byte, 14)
	n := hex.Encode(dst, src)
	fmt.Println(n, string(dst[:n]), dst[12:])

	for _, in := range []string{"00ab", "DEADbeef", "", "abc", "zz", "a", "0g", "g0", "0x12", "abz", "ab\xff"} {
		decode(in)
	}
	out := make([]byte, 4)
	m, err := hex.Decode(out, []byte("01020z04"))
	fmt.Println(m, out, err)
	fmt.Printf("%v %T %d\n", hex.InvalidByteError('\n'), hex.InvalidByteError('é'), hex.InvalidByteError('a'))

	attempt("short encode", func() { hex.Encode(make([]byte, 3), []byte{1, 2}) })
	attempt("short decode", func() { hex.Decode(make([]byte, 1), []byte("0102")) })
}
