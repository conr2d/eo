package main

import (
	"fmt"
	"strconv"
	"strings"
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

var scales = []float64{5e-324, 1e-310, 1e-20, 1e-5, 0.001, 0.1, 1, 10, 1000, 1e5, 1e6, 1e15, 1e21, 1e300}

var specials = []float64{0, 0.5, 1.5, 2.5, 1.005, 0.125, 1e23, 123456789, 9.999999, 0.1, 100}

var words = []string{"", "a", "hello", "héllo", "☺x", "tab\tq", "`b`", "\xff", "quo\"te", "日本語"}

var verbs = []string{"v", "d", "x", "X", "o", "b", "e", "E", "f", "F", "g", "G", "s", "q", "c", "U", "t"}

var floatFormats = []byte{'b', 'e', 'E', 'f', 'g', 'G', 'x', 'X'}

func randFloat() float64 {
	var f float64
	if pick(4) == 0 {
		f = specials[pick(len(specials))]
	} else {
		f = float64(next()>>11) / 9007199254740992 * scales[pick(len(scales))]
	}
	if pick(2) == 0 {
		f = -f
	}
	return f
}

func randFormat() string {
	var b strings.Builder
	b.WriteString("%")
	for _, flag := range []string{"#", "0", "+", "-", " "} {
		if pick(4) == 0 {
			b.WriteString(flag)
		}
	}
	if pick(2) == 0 {
		b.WriteString(strconv.Itoa(pick(14)))
	}
	switch pick(3) {
	case 0:
		b.WriteString("." + strconv.Itoa(pick(22)))
	case 1:
		if pick(4) == 0 {
			b.WriteString(".")
		}
	}
	b.WriteString(verbs[pick(len(verbs))])
	return b.String()
}

func main() {
	for i := 0; i < 400; i++ {
		f := randFloat()
		format := randFormat()
		fmt.Printf("%d %s [", i, format)
		switch pick(7) {
		case 0:
			fmt.Printf(format, f)
		case 1:
			fmt.Printf(format, float32(f))
		case 2:
			// Odd values, so that %c never prints a NUL.
			fmt.Printf(format, int(next()|1))
		case 3:
			fmt.Printf(format, uint(next()>>uint(pick(64))|1))
		case 4:
			fmt.Printf(format, int8(next()|1))
		case 5:
			fmt.Printf(format, words[pick(len(words))])
		default:
			fmt.Printf(format, []byte(words[pick(len(words))]))
		}
		fmt.Println("]")
		ff := floatFormats[pick(len(floatFormats))]
		prec := pick(26) - 1
		bits := 64
		if pick(3) == 0 {
			bits = 32
		}
		fmt.Println(string(ff), prec, bits, strconv.FormatFloat(f, ff, prec, bits))
	}
}
