package main

import (
	"bytes"
	"encoding"
	"encoding/base64"
	"errors"
	"fmt"
	"io"
	"slices"
	"strconv"
	"strings"
	"unicode/utf16"
)

// A linear congruential generator, so that both programs draw the same values.
var state uint64 = 88172645463325252

func next() uint64 {
	state = state*6364136223846793005 + 1442695040888963407
	return state
}

func pick(n int) int {
	return int(next() >> 33 % uint64(n))
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

// A reader that returns at most n bytes per call.
type chunkReader struct {
	r io.Reader
	n int
}

func (c *chunkReader) Read(p []byte) (int, error) {
	if len(p) > c.n {
		p = p[:c.n]
	}
	return c.r.Read(p)
}

// A writer that fails after limit bytes.
type failWriter struct {
	buf   bytes.Buffer
	limit int
}

var errFull = errors.New("full")

func (w *failWriter) Write(p []byte) (int, error) {
	if w.buf.Len()+len(p) > w.limit {
		return 0, errFull
	}
	return w.buf.Write(p)
}

type Celsius int

func (c Celsius) MarshalText() ([]byte, error) {
	return []byte(fmt.Sprintf("%dC", int(c))), nil
}

func (c *Celsius) UnmarshalText(text []byte) error {
	digits, ok := strings.CutSuffix(string(text), "C")
	if !ok {
		return errors.New("no unit")
	}
	n, err := strconv.Atoi(digits)
	if err != nil {
		return err
	}
	*c = Celsius(n)
	return nil
}

type Blob struct {
	data []byte
}

func (b Blob) MarshalBinary() ([]byte, error) {
	return append([]byte{byte(len(b.data))}, b.data...), nil
}

func (b *Blob) UnmarshalBinary(data []byte) error {
	if len(data) == 0 || int(data[0]) != len(data)-1 {
		return errors.New("bad blob")
	}
	b.data = data[1:]
	return nil
}

func (b Blob) AppendBinary(dst []byte) ([]byte, error) {
	return append(append(dst, byte(len(b.data))), b.data...), nil
}

func (b Blob) AppendText(dst []byte) ([]byte, error) {
	return base64.StdEncoding.AppendEncode(dst, b.data), nil
}

func decode(name string, enc *base64.Encoding, s string) {
	b, err := enc.DecodeString(s)
	var cie base64.CorruptInputError
	isCorrupt := errors.As(err, &cie)
	fmt.Printf("%s %q -> %q %v %v %d\n", name, s, b, err, isCorrupt, int64(cie))
}

func encodings() {
	inputs := []string{"", "f", "fo", "foo", "foob", "fooba", "foobar", "\xfb\xff\xbf", "\x00\x00\x00\x00\x00\x00\x00\x00\x00"}
	for _, in := range inputs {
		src := []byte(in)
		fmt.Println(base64.StdEncoding.EncodeToString(src), base64.URLEncoding.EncodeToString(src),
			base64.RawStdEncoding.EncodeToString(src), base64.RawURLEncoding.EncodeToString(src),
			base64.StdEncoding.EncodedLen(len(src)), base64.RawStdEncoding.EncodedLen(len(src)))
	}
	custom := base64.NewEncoding("ZYXWVUTSRQPONMLKJIHGFEDCBAzyxwvutsrqponmlkjihgfedcba9876543210+/").WithPadding('*')
	s := custom.EncodeToString([]byte("custom!"))
	b, err := custom.DecodeString(s)
	fmt.Println(s, string(b), err, custom.DecodedLen(len(s)), base64.RawURLEncoding.DecodedLen(11))

	dst := make([]byte, 12)
	base64.StdEncoding.Encode(dst, []byte("hello"))
	fmt.Println(dst, string(dst[:8]))
	out := base64.StdEncoding.AppendEncode([]byte("x="), []byte("hi there"))
	fmt.Println(string(out))
	dec, err := base64.URLEncoding.AppendDecode([]byte("y:"), []byte("aGk_Pz8-"))
	fmt.Printf("%q %v\n", dec, err)
	dec, err = base64.RawStdEncoding.AppendDecode(nil, []byte("aGk/Pz8+a"))
	fmt.Printf("%q %v\n", dec, err)
	n, err := base64.StdEncoding.Decode(make([]byte, 30), []byte("Zm9vYmFyYmF6cXV4\r\nZm9v"))
	fmt.Println(n, err)
}

func decodeErrors() {
	cases := []string{
		"Zm9v", "Zm9vYg==", "Zm9vYg=", "Zm9vYg", "Zm9vYmE=", "Zm9vYmE", "Zm9v\nYmFy", "Zm9v\r\nYmFy\n",
		"Zm9vYg==\n", "Zm9vYg==x", "Zm9vYg=\n=", "Zm9vYg\n==", "=", "Z", "Zm=", "Zm9=v", "Zm9v!", "Zm9vYmFy!xyz",
		"Zm9vYmFyYmF6", "Zm9vYmFyYmF6=", "Zh==", "Zm9=", "Zm8=", "\n\n", "Zm9vYmFyYmF6cXV4YQ==", "Zm9vYmFyYmF6cXV4!Q==",
	}
	for _, c := range cases {
		decode("std", base64.StdEncoding, c)
	}
	for _, c := range []string{"Zh", "Zm9", "Zm8", "Zm9vYg", "Zm9vYg==", "aGk_", "aGk/"} {
		decode("rawurl", base64.RawURLEncoding, c)
	}
	strict := base64.StdEncoding.Strict()
	for _, c := range []string{"Zh==", "Zm9=", "Zm8=", "Zm9vYg==", "Zm9vYh==", "Zm9vYmF=", "Zm9vYmE="} {
		decode("strict", strict, c)
	}
	rawStrict := base64.RawStdEncoding.Strict()
	for _, c := range []string{"Zh", "Zm9", "Zm8"} {
		decode("rawstrict", rawStrict, c)
	}
	var err error = base64.CorruptInputError(42)
	fmt.Printf("%v|%T|%d|%s\n", err, err, base64.CorruptInputError(7), base64.CorruptInputError(-1))
}

func streams() {
	data := []byte("The quick brown fox jumps over the lazy dog, again and again and again.")
	for _, enc := range []*base64.Encoding{base64.StdEncoding, base64.RawURLEncoding} {
		var buf bytes.Buffer
		w := base64.NewEncoder(enc, &buf)
		for i := 0; i < len(data); {
			k := 1 + pick(9)
			if i+k > len(data) {
				k = len(data) - i
			}
			n, err := w.Write(data[i : i+k])
			if n != k || err != nil {
				fmt.Println("short write", n, err)
			}
			i += k
		}
		fmt.Println(buf.Len(), w.Close(), w.Close())
		encoded := buf.String()
		fmt.Println(encoded == enc.EncodeToString(data), encoded[:20])

		wrapped := encoded[:10] + "\n" + encoded[10:30] + "\r\n" + encoded[30:]
		for _, chunk := range []int{1, 3, 4, 7, 1000} {
			r := base64.NewDecoder(enc, &chunkReader{strings.NewReader(wrapped), chunk})
			var got []byte
			p := make([]byte, 1+chunk%5)
			for {
				n, err := r.Read(p)
				got = append(got, p[:n]...)
				if err != nil {
					fmt.Println(chunk, bytes.Equal(got, data), len(got), err)
					break
				}
			}
		}
	}
	all, err := io.ReadAll(base64.NewDecoder(base64.StdEncoding, strings.NewReader("Zm9vYmFy!Zm9v")))
	fmt.Printf("%q %v\n", all, err)
	all, err = io.ReadAll(base64.NewDecoder(base64.StdEncoding, strings.NewReader("Zm9vYmE")))
	fmt.Printf("%q %v\n", all, err)
	all, err = io.ReadAll(base64.NewDecoder(base64.RawStdEncoding, strings.NewReader("Zm9vYmE")))
	fmt.Printf("%q %v\n", all, err)
	all, err = io.ReadAll(base64.NewDecoder(base64.RawStdEncoding, strings.NewReader("Zm9vY")))
	fmt.Printf("%q %v\n", all, err)

	fw := &failWriter{limit: 8}
	w := base64.NewEncoder(base64.StdEncoding, fw)
	n, err := w.Write([]byte("abcdefghi"))
	fmt.Println(n, err, fw.buf.String())
	n, err = w.Write([]byte("j"))
	fmt.Println(n, err, w.Close())
	fw2 := &failWriter{limit: 4}
	w2 := base64.NewEncoder(base64.StdEncoding, fw2)
	w2.Write([]byte("abcd"))
	fmt.Println(w2.Close(), fw2.buf.String())
	fmt.Printf("%T %T\n", w, base64.NewDecoder(base64.StdEncoding, strings.NewReader("")))
}

func randomRounds() {
	encs := []*base64.Encoding{base64.StdEncoding, base64.URLEncoding, base64.RawStdEncoding, base64.RawURLEncoding,
		base64.StdEncoding.Strict(), base64.RawURLEncoding.Strict()}
	symbols := "AZaz09+/-_=\n\r!Q"
	for round := 0; round < 300; round++ {
		enc := encs[pick(len(encs))]
		src := make([]byte, pick(20))
		for i := range src {
			src[i] = byte(next() >> 40)
		}
		s := enc.EncodeToString(src)
		back, err := enc.DecodeString(s)
		if err != nil || !bytes.Equal(back, src) {
			fmt.Println("round trip failed", round, s, err)
		}
		mutated := []byte(s)
		for k := pick(3); k > 0 && len(mutated) > 0; k-- {
			i := pick(len(mutated) + 1)
			c := symbols[pick(len(symbols))]
			if i == len(mutated) {
				mutated = append(mutated, c)
			} else if pick(2) == 0 {
				mutated[i] = c
			} else {
				mutated = append(mutated[:i], append([]byte{c}, mutated[i:]...)...)
			}
		}
		dst := make([]byte, enc.DecodedLen(len(mutated))+pick(3))
		n, err := enc.Decode(dst, mutated)
		fmt.Printf("%d %q %d %x %v\n", round, mutated, n, dst[:n], err)
	}
}

func utf16Package() {
	for _, r := range []rune{0, 'a', 0xd7ff, 0xd800, 0xdbff, 0xdc00, 0xdfff, 0xe000, 0xfffd, 0xffff, 0x10000, 0x1f600, 0x10ffff, 0x110000, -1} {
		r1, r2 := utf16.EncodeRune(r)
		fmt.Printf("%#x %v %#x %#x %#x %d %v\n", r, utf16.IsSurrogate(r), r1, r2, utf16.DecodeRune(r1, r2), utf16.RuneLen(r),
			utf16.AppendRune([]uint16{1}, r))
	}
	fmt.Printf("%#x %#x %#x\n", utf16.DecodeRune(0xdc00, 0xd800), utf16.DecodeRune(0xd800, 'a'), utf16.DecodeRune('a', 'b'))
	enc := utf16.Encode([]rune("aé\U0001F600\U0010FFFF"))
	enc2 := utf16.Encode([]rune{-5, 0xd800, 0x110000, 'z'})
	fmt.Println(enc, enc2, len(enc), cap(enc) >= len(enc))
	fmt.Println(string(utf16.Decode(enc)), utf16.Decode([]uint16{0xd800}), utf16.Decode([]uint16{0xd800, 'a', 0xdc00, 0xdbff, 0xdfff}),
		utf16.Decode(nil) == nil, len(utf16.Decode(nil)))
}

func encodingPackage() {
	var vals = []any{Celsius(21), new(Celsius), Blob{[]byte("ab")}, &Blob{}, 3}
	for _, v := range vals {
		tm, isTM := v.(encoding.TextMarshaler)
		_, isTU := v.(encoding.TextUnmarshaler)
		_, isBM := v.(encoding.BinaryMarshaler)
		_, isBU := v.(encoding.BinaryUnmarshaler)
		_, isBA := v.(encoding.BinaryAppender)
		_, isTA := v.(encoding.TextAppender)
		fmt.Printf("%T %v %v %v %v %v %v", v, isTM, isTU, isBM, isBU, isBA, isTA)
		if isTM {
			text, err := tm.MarshalText()
			fmt.Printf(" %s %v", text, err)
		}
		fmt.Println()
	}
	var c Celsius
	var tu encoding.TextUnmarshaler = &c
	fmt.Println(tu.UnmarshalText([]byte("37C")), c, tu.UnmarshalText([]byte("hot")) != nil)
	var b Blob
	var bu encoding.BinaryUnmarshaler = &b
	var bm encoding.BinaryMarshaler = Blob{[]byte("xyz")}
	raw, err := bm.MarshalBinary()
	fmt.Println(raw, err, bu.UnmarshalBinary(raw), string(b.data), bu.UnmarshalBinary([]byte{9}))
	var ba encoding.BinaryAppender = b
	var ta encoding.TextAppender = b
	x, err := ba.AppendBinary([]byte{0xee})
	y, err2 := ta.AppendText([]byte("t:"))
	fmt.Println(x, err, string(y), err2)
	fmt.Printf("%T %v\n", tu, tu != nil)
}

func grow() {
	s := []int{1, 2, 3}
	g := slices.Grow(s[:2], 1)
	g = append(g, 9)
	fmt.Println(s, g, cap(g) >= 3)
	h := slices.Grow(s, 10)
	h = append(h, 4)
	fmt.Println(s, h, cap(h) >= 13, len(h))
	var nilS []int
	fmt.Println(slices.Grow(nilS, 0) == nil, len(slices.Grow(nilS, 5)), cap(slices.Grow(nilS, 5)) >= 5)
	attempt("grow negative", func() { _ = slices.Grow(s, -1) })
}

func main() {
	encodings()
	decodeErrors()
	streams()
	randomRounds()
	utf16Package()
	encodingPackage()
	grow()
	attempt("short alphabet", func() { base64.NewEncoding("abc") })
	attempt("newline alphabet", func() {
		base64.NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+\n")
	})
	attempt("duplicate alphabet", func() {
		base64.NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+A")
	})
	attempt("padding newline", func() { base64.StdEncoding.WithPadding('\n') })
	attempt("padding range", func() { base64.StdEncoding.WithPadding(0x100) })
	attempt("padding negative", func() { base64.StdEncoding.WithPadding(-2) })
	attempt("padding in alphabet", func() { base64.StdEncoding.WithPadding('A') })
	attempt("short dst", func() { base64.StdEncoding.Encode(make([]byte, 3), []byte("abcd")) })
	attempt("short decode dst", func() { base64.StdEncoding.Decode(make([]byte, 2), []byte("Zm9v")) })
	attempt("nil encoding", func() {
		var enc *base64.Encoding
		enc.EncodeToString([]byte("a"))
	})
	fmt.Println(base64.RawStdEncoding.EncodeToString([]byte("ok")), base64.StdEncoding.EncodeToString(nil) == "")
}
