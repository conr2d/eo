package main

import (
	"errors"
	"fmt"
	"strconv"
)

var zero float64

type Celsius float64

type Count int

type Name string

type Hashes []uint8

type point struct {
	X, Y int
}

type item struct {
	Name  string
	Pos   point
	Tags  []string
	Err   error
	Value any
}

type label struct {
	text string
}

func (l label) String() string { return "<" + l.text + ">" }

type code int

func (c code) GoString() string { return "code(" + strconv.Itoa(int(c)) + ")" }

type boom struct{}

func (boom) String() string { panic("boom") }

func main() {
	nan, inf, negZero := zero/zero, 1/zero, -zero
	pi := 3.141592653589793
	// Integers with widths, flags, and precisions.
	fmt.Printf("[%5d] [%-5d] [%05d] [%+d] [% d] [%+05d] [%-+5d]\n", 42, 42, 42, 42, 42, -42, 7)
	fmt.Printf("[%x] [%X] [%#x] [%#X] [%#o] [%O] [%b] [%#b] [%o]\n", 255, 255, 255, 255, 8, 8, 5, 5, -8)
	fmt.Printf("[%08.3d] [%.0d] [%5.0d] [%.3x] [%-#10x] [%#08x] [%x]\n", 7, 0, 0, 10, 255, 255, -255)
	fmt.Printf("[%d] [%x] [%#o] [%03d] [%3d]\n", int64(-1<<63), ^uint64(0), 0, -5, 12345)
	fmt.Printf("[%c] [%5c] [%-5c] [%q] [%+q] [%5q] [%c]\n", 'A', 'é', 'z', 'é', 'é', 'x', 0x110000)
	fmt.Printf("[%U] [%#U] [%.6U] [%#U] [%10U] [%-10U]\n", 0x1F600, 'x', 0x41, '\n', 'a', 'a')
	fmt.Printf("[%6v] [%-6v] [%06v] [%v]\n", Count(3), Count(3), uint8(9), Count(-1))

	// Strings and bytes.
	fmt.Printf("[%10s] [%-10s] [%.3s] [%.2s] [%10.2s] [%05s]\n", "hello", "hello", "hello", "héllo", "héllo", "ab")
	fmt.Printf("[%q] [%+q] [%#q] [%#q] [%#+q] [%.3q]\n", "héllo\n", "héllo", "back`tick", "plain", "é", "abcdef")
	fmt.Printf("[%x] [% x] [%#x] [% #x] [%X] [%.2x] [%8x] [%-8x] [%x]\n", "abc", "abc", "abc", "abc", "abc", "abc", "ab", "ab", "")
	fmt.Printf("[%s] [%x] [% X] [%q] [%5s] [%v] [%d] [%3v]\n", []byte("hi"), []byte("hi"), []byte("hi"), []byte("hi"), []byte("hi"), []byte("hi"), []byte("hi"), []byte{1, 2})
	fmt.Printf("[%8v] [%-8v] [%8q] [%x]\n", Name("go"), Name("go"), Name("go"), Name("go"))

	// Floats.
	fmt.Printf("[%.2f] [%8.3f] [%-8.2f] [%+.1f] [% .1f] [%08.2f] [%-08.2f] [%+08.2f] [%08.2f]\n", 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, 3.14159, -3.14159)
	fmt.Printf("[%e] [%.3e] [%E] [%g] [%.3g] [%G] [%.10g] [%g] [%g]\n", 123456.789, 123456.789, 0.000123, 123456.789, 123456.789, 1e-7, 1.0/3, 1e21, 100000.0)
	fmt.Printf("[%#g] [%#.3g] [%#.0f] [%#e] [%#.0e] [%#g] [%#v] [%#g]\n", 1.0, 2.0, 3.0, 4.0, 5.0, 123456789.0, 1.5, 0.0)
	fmt.Printf("[%5.2f] [%+f] [%05f] [% f] [%010f] [%-10f] [%f] [%+.1e]\n", nan, nan, nan, nan, inf, -inf, -inf, inf)
	fmt.Printf("[%.0f] [%.0f] [%.0f] [%.1f] [%.2f] [%.3f] [%.20f]\n", 0.5, 1.5, 2.5, 0.25, 1.005, -0.0005, 0.1)
	fmt.Printf("[%v] [%v] [%v] [%v] [%v] [%v] [%.3v] [%10.4v]\n", 1e20, 1e21, 1e-5, 123456789.0, 0.000012345, negZero, pi, pi)
	fmt.Printf("[%b] [%x] [%X] [%.3x] [%#x] [%x] [%.0x] [%x]\n", 1.0, 1.0, -0.5, pi, 1.0, 0.0, 1.5, 5e-324)
	fmt.Printf("[%v] [%g] [%.3f] [%e] [%b] [%x]\n", float32(0.1), float32(1e10), float32(2.5), float32(1)/3, float32(1), float32(0.1))
	fmt.Printf("[%6.2f] [%v] [%.1f] [%F] [%e]\n", Celsius(21.456), Celsius(-3), Celsius(0.05), 1.5, 0.0)

	// Booleans, nil, and types.
	fmt.Printf("[%6t] [%-6t] [%6v] [%5v] [%-5v] [%d] [%10T] [%-10T]\n", true, false, true, nil, nil, nil, 3, "s")

	// Widths and precisions from operands.
	fmt.Printf("[%*d] [%-*d] [%*d] [%.*f] [%*.*f] [%.*s]\n", 5, 42, 5, 42, -5, 42, 2, pi, 8, 3, pi, 2, "hello")
	fmt.Printf("[%*d] [%.*d] [%*d] [%.*d]\n", "x", 1, "y", 2, 10000000, 3, -1, 4)
	fmt.Printf("[%*d] [%*d]\n", uint8(3), 1, Count(4), 2)
	fmt.Printf("[%*d]\n", 3)

	// Operand indexes.
	fmt.Printf("[%[2]d %[1]d] [%[1]d %d] [%[3]*.[2]*[1]f] [%[5]d] [%[0]d] [%[x]d] [%[1]]\n", 1, 2, 9)
	fmt.Printf("[%[2]d]\n", 1, 2, 3)
	fmt.Printf("[%d %d]\n", 1, 2, 3, "x", nil)
	fmt.Printf("[%-]\n")
	fmt.Printf("[%[1]5d]\n", 3)
	fmt.Printf("%[3]v %[1]v %v\n", "a", "b", "c")

	// Structs with field names, and Go syntax.
	p := point{1, -2}
	it := item{Name: "box", Pos: point{3, 4}, Tags: []string{"a", "b"}}
	fmt.Printf("%v | %+v | %#v\n", p, p, p)
	fmt.Printf("%+v\n%#v\n", it, it)
	fmt.Printf("%+v | %#v | %+v\n", &p, &p, []point{{1, 2}, {3, 4}})
	fmt.Printf("[%6v] [%-4v] [%+5v]\n", p, p, p)
	fmt.Printf("%#v %#v %#v %#v %#v %#v\n", 42, uint(42), "q\"s", true, 1e6, int8(-3))
	fmt.Printf("%#v %#v %#v %#v\n", []int{1, 2}, []string(nil), []int{}, [3]int{1, 2, 3})
	fmt.Printf("%#v %#v %#v\n", map[string]int{"b": 2, "a": 1}, map[string]int(nil), map[int]bool{})
	fmt.Printf("%#v %#v %#v %#v\n", []byte{1, 2}, []byte(nil), Hashes{0xab}, Hashes(nil))
	fmt.Printf("%#v %#v %#v %#v\n", Count(5), Name("n"), Celsius(1.5), [2]uint8{1, 2})
	fmt.Printf("%#v %#v %v %+v\n", code(7), []code{1, 2}, code(7), label{"x"})
	fmt.Printf("%#v | %#v\n", any(nil), []error{nil})
	fmt.Printf("%#v\n", []any{1, "a", nil, point{}})

	// Methods with flags.
	l := label{"hi"}
	err := errors.New("bad thing")
	fmt.Printf("[%10v] [%-10s] [%.3s] [%x] [%q] [%8v] [%-12v]\n", l, l, l, l, l, err, err)
	fmt.Printf("[%10v] [%-8d]\n", boom{}, label{"x"})
	fmt.Printf("[%5z] [%-6z] [%z]\n", 3, "ab", l)
	fmt.Printf("[%p] [%p] [%p] [%p] [%v]\n", (*int)(nil), []int(nil), map[int]int(nil), 5, (*int)(nil))
	fmt.Printf("[%6.2f%%] [%5%] [%-5%]\n", 99.5)
	wrapped := fmt.Errorf("op %[2]s: %[1]w", err, "read")
	fmt.Println(wrapped, errors.Is(wrapped, err))
	fmt.Println(fmt.Sprintf("%08.3f|%-8.3f|%+.2e", -1.5, 2.25, 12345.678))

	// strconv's float formatting and ASCII quoting.
	for _, f := range []byte{'b', 'e', 'E', 'f', 'g', 'G', 'x', 'X', 'z'} {
		fmt.Println(string(f), strconv.FormatFloat(1234.5678, f, -1, 64), strconv.FormatFloat(1234.5678, f, 2, 64), strconv.FormatFloat(-0.000123, f, 3, 32))
	}
	fmt.Println(strconv.FormatFloat(1e23, 'g', -1, 64), strconv.FormatFloat(5e-324, 'e', -1, 64), strconv.FormatFloat(0.1, 'f', 30, 32), strconv.FormatFloat(123, 'g', 2, 64))
	fmt.Println(strconv.FormatFloat(1.7976931348623157e308, 'f', 0, 64))
	fmt.Println(string(strconv.AppendFloat([]byte("x="), 2.5, 'f', 1, 64)), strconv.FormatFloat(0, 'g', 3, 64), strconv.FormatFloat(100, 'g', 1, 64), strconv.FormatFloat(1e-5, 'g', 1, 64))
	fmt.Println(strconv.QuoteToASCII("héllo\x01☺"), strconv.QuoteRuneToASCII('☺'), strconv.QuoteRuneToASCII('a'), strconv.QuoteRune('☺'))
	fmt.Println(strconv.CanBackquote("plain text"), strconv.CanBackquote("tab\there"), strconv.CanBackquote("new\nline"), strconv.CanBackquote("`"), strconv.CanBackquote("\ufeff"), strconv.CanBackquote("\xff"))
	defer func() { fmt.Println("recovered:", recover()) }()
	strconv.FormatFloat(1, 'g', -1, 16)
}
