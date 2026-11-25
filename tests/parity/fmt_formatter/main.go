package main

import (
	"bytes"
	"encoding/hex"
	"errors"
	"fmt"
	"math/big"
	"strings"
)

// Hash formats itself as go-ethereum's common.Hash does.
type Hash [4]byte

func (h Hash) Format(s fmt.State, c rune) {
	hexb := make([]byte, 2+len(h)*2)
	copy(hexb, "0x")
	hex.Encode(hexb[2:], h[:])

	switch c {
	case 'x', 'X':
		if !s.Flag('#') {
			hexb = hexb[2:]
		}
		if c == 'X' {
			hexb = bytes.ToUpper(hexb)
		}
		fallthrough
	case 'v', 's':
		s.Write(hexb)
	case 'q':
		q := []byte{'"'}
		s.Write(q)
		s.Write(hexb)
		s.Write(q)
	case 'd':
		fmt.Fprint(s, ([len(h)]byte)(h))
	default:
		fmt.Fprintf(s, "%%!%c(hash=%x)", c, h)
	}
}

// echo prints the directive that it was formatted with.
type echo struct{}

func (echo) Format(s fmt.State, verb rune) {
	w, wok := s.Width()
	p, pok := s.Precision()
	fmt.Fprintf(s, "%s[%d %t %d %t %T]", fmt.FormatString(s, verb), w, wok, p, pok, s)
}

// counter has a pointer receiver.
type counter struct{ n int }

func (c *counter) Format(s fmt.State, verb rune) {
	c.n++
	fmt.Fprintf(s, "counter(%d)", c.n)
}

// boom panics in Format after writing.
type boom struct{}

func (boom) Format(s fmt.State, verb rune) {
	s.Write([]byte("partial"))
	panic("boom")
}

// nilsafe has a value receiver, which a nil pointer cannot call.
type nilsafe struct{ v int }

func (n nilsafe) Format(s fmt.State, verb rune) {
	fmt.Fprintf(s, "nilsafe(%d)", n.v)
}

// fe is an error that is also a Formatter.
type fe struct{ code int }

func (e fe) Error() string { return fmt.Sprintf("fe %d", e.code) }

func (e fe) Format(s fmt.State, verb rune) {
	fmt.Fprintf(s, "fe<%c %d>", verb, e.code)
}

type holder struct {
	H Hash
	h Hash
}

// named prints by String, except where Go cannot call it: in an unexported field.
type named struct{ s string }

func (n named) String() string { return "named:" + n.s }

type inner struct{ N named }

type wrap struct {
	Pub  named
	priv named
	in   inner
	In   inner
	v    fmt.Stringer
	V    fmt.Stringer
	list []named
}

func main() {
	h := Hash{0xde, 0xad, 0xbe, 0xef}
	fmt.Printf("%v|%s|%x|%X|%#x|%#X|%q|%d|%e\n", h, h, h, h, h, h, h, h, h)
	fmt.Println(h, []Hash{h, {1, 2, 3, 4}})
	fmt.Printf("%v|%+v\n", holder{h, h}, holder{h, h})
	w := wrap{named{"a"}, named{"b"}, inner{named{"c"}}, inner{named{"d"}}, named{"e"}, named{"f"}, []named{{"g"}}}
	fmt.Printf("%v\n%+v\n", w, w)
	var f fmt.Formatter = h
	fmt.Printf("%v %T\n", f, f)
	fmt.Println(fmt.Sprint(h), fmt.Sprintf("%10v|", h))

	for _, d := range []string{"%v", "%+v", "%#v", "%-8.3x", "% 05d", "%+#q", "%*.*s", "%.v", "%5c", "%[2]*[1]d"} {
		switch d {
		case "%*.*s":
			fmt.Println(fmt.Sprintf(d, 7, 2, echo{}))
		case "%[2]*[1]d":
			fmt.Println(fmt.Sprintf(d, echo{}, -4))
		default:
			fmt.Println(fmt.Sprintf(d, echo{}))
		}
	}

	c := &counter{}
	fmt.Println(c, c)
	fmt.Printf("%d %v\n", c, *c)

	fmt.Printf("%v|%8d|\n", boom{}, boom{})
	var np *nilsafe
	fmt.Printf("%v|%8v|%-6s|%.2v|\n", np, np, np, np)
	fmt.Println(&nilsafe{7}, nilsafe{8})

	err := fmt.Errorf("wrapped: %w", fe{3})
	var target fe
	fmt.Println(err.Error(), errors.As(err, &target), target.code)
	fmt.Printf("%v %s %d\n", fe{4}, fe{5}, fe{6})

	// big.Int's Format honors flags, width, and precision.
	x := big.NewInt(255)
	neg := big.NewInt(-42)
	zero := new(big.Int)
	for _, d := range []string{"%d", "%08d", "%+d", "% d", "%-8d|", "%8d|", "%.6d", "%10.5x|", "%#x", "%#X", "%#b", "%#o", "%O", "%x", "%X", "%s", "%v", "%+v", "%q", "%08.3d", "%-08d|"} {
		fmt.Printf(d+" "+d+" "+d+"\n", x, neg, zero)
	}
	fmt.Printf("[%.0d] [%.d] [%5.0d] [%.0d]\n", zero, zero, zero, x)
	var nilInt *big.Int
	fmt.Printf("%d|%10d|%x|%q\n", nilInt, nilInt, nilInt, nilInt)
	fmt.Println(x, neg, []*big.Int{x, zero})
	fmt.Printf("%v|%+v|%d|%x\n", *x, *x, *neg, *x)

	// Fprint, Fprintln, and Fprintf write to any io.Writer.
	var sb strings.Builder
	n, werr := fmt.Fprint(&sb, "a", 1, 2, "b", 3.5, true)
	fmt.Println(n, werr, sb.String())
	sb.Reset()
	n, werr = fmt.Fprintln(&sb, "a", 1, 2, "b")
	fmt.Printf("%d %v %q\n", n, werr, sb.String())
	var buf bytes.Buffer
	n, werr = fmt.Fprintf(&buf, "%05d|%x|%v", 42, "hi", h)
	fmt.Println(n, werr, buf.String())

	// Append, Appendln, and Appendf append to a []byte.
	b := []byte("start:")
	b = fmt.Append(b, "x", 1, 2)
	b = fmt.Appendf(b, "[%03d]", 7)
	b = fmt.Appendln(b, "end", h)
	fmt.Printf("%q %d\n", b, len(b))
	fmt.Println(string(fmt.Append(nil)), fmt.Append(nil) == nil, len(fmt.Appendf(nil, "")))

	// Print and Println return the count of bytes written.
	n, werr = fmt.Print("print", 1, 2, "\n")
	fmt.Println(n, werr)
	n, werr = fmt.Println("println", 1, 2)
	fmt.Println(n, werr)
	n, werr = fmt.Printf("%s %d\n", "printf", 3)
	fmt.Println(n, werr)
}
