package main

import (
	"cmp"
	"errors"
	"fmt"
	"math"
	"slices"
	"strconv"
	"strings"
	"unicode"
)

// A linear congruential generator, so that both programs draw the same values.
var state uint64 = 88172645463325252

func next() uint64 {
	state = state*6364136223846793005 + 1442695040888963407
	return state
}

func pick(n int) int {
	return int((next() >> 33) % uint64(n))
}

type Celsius float64

type Name string

var specials = []float64{0, math.Copysign(0, -1), 1, -1, 0.5, -0.5, 1.5, 2.5, -2.5, 3.5, 0.49999999999999994, 4503599627370495.5, 4503599627370497, 1e300, -1e-300, 5e-324, math.MaxFloat64, math.Inf(1), math.Inf(-1), math.NaN()}

func constants() {
	fmt.Println("E", math.E, "Pi", math.Pi, "Phi", math.Phi)
	fmt.Println("Sqrt2", math.Sqrt2, "SqrtE", math.SqrtE, "SqrtPi", math.SqrtPi, "SqrtPhi", math.SqrtPhi)
	fmt.Println("Ln2", math.Ln2, "Log2E", math.Log2E, "Ln10", math.Ln10, "Log10E", math.Log10E)
	fmt.Println("MaxFloat32", math.MaxFloat32, "SmallestNonzeroFloat32", math.SmallestNonzeroFloat32)
	fmt.Println("MaxFloat64", math.MaxFloat64, "SmallestNonzeroFloat64", math.SmallestNonzeroFloat64)
	fmt.Println(math.MaxInt, math.MinInt, math.MaxInt8, math.MinInt8, math.MaxInt16, math.MinInt16)
	fmt.Println(math.MaxInt32, math.MinInt32, math.MaxInt64, math.MinInt64)
	fmt.Println(uint64(math.MaxUint), math.MaxUint8, math.MaxUint16, math.MaxUint32, uint64(math.MaxUint64))
	var n int64 = 1 << 40
	fmt.Println(n > math.MaxUint32, int32(n>>20) < math.MaxInt16, float32(math.MaxFloat32))
}

func bitsAndSigns() {
	fmt.Printf("%#x %#x %#x\n", math.Float64bits(math.NaN()), math.Float64bits(math.Inf(1)), math.Float64bits(math.Inf(-7)))
	fmt.Printf("%#x %v\n", math.Float32bits(float32(1.5)), math.Float32frombits(0x7f7fffff))
	fmt.Println(math.Float64frombits(math.Float64bits(-2.75)), math.Float64frombits(1))
	for _, x := range specials {
		fmt.Println(x, math.Abs(x), math.Signbit(x), math.Copysign(3, x), math.Copysign(x, -1),
			math.IsNaN(x), math.IsInf(x, 0), math.IsInf(x, 1), math.IsInf(x, -1))
	}
}

func rounding() {
	for _, x := range specials {
		fmt.Println(x, math.Floor(x), math.Ceil(x), math.Trunc(x), math.Round(x), math.RoundToEven(x))
	}
	for i := 0; i < 200; i++ {
		x := float64(int64(next()>>12)-1<<51) / float64(int64(1)<<uint(pick(60)))
		if pick(4) == 0 {
			x = float64(pick(2000)-1000) / 2
		}
		fmt.Printf("%x %x %x %x %x %x\n", x, math.Floor(x), math.Ceil(x), math.Trunc(x), math.Round(x), math.RoundToEven(x))
	}
}

func minMax() {
	negZero := math.Copysign(0, -1)
	nan := math.NaN()
	inf := math.Inf(1)
	pairs := [][2]float64{{1, 2}, {2, 1}, {0, negZero}, {negZero, 0}, {nan, 1}, {1, nan}, {inf, nan}, {nan, -inf}, {-1, -inf}, {3, 3}}
	for _, p := range pairs {
		x := p[0]
		y := p[1]
		fmt.Println(x, y, math.Max(x, y), math.Min(x, y), math.Dim(x, y), math.Mod(x, y), math.Nextafter(x, y))
	}
	fmt.Println(math.Mod(7, -3), math.Mod(-7, 3), math.Mod(math.Inf(1), 2), math.Mod(2, math.Inf(-1)), math.Mod(1, 0))
	fmt.Println(math.Nextafter(0, -1), math.Nextafter(1, 2), math.Nextafter(math.MaxFloat64, math.Inf(1)))
	_, posFrac := math.Modf(math.Inf(1))
	_, negFrac := math.Modf(math.Inf(-1))
	fmt.Printf("%#x %#x %#x\n", math.Float64bits(posFrac), math.Float64bits(negFrac), math.Float64bits(math.Sqrt(-1)))
	fmt.Printf("%#x %#x %#x %#x\n", math.Float64bits(math.Mod(1, 0)), math.Float64bits(math.Mod(math.Inf(-1), 1)), math.Float64bits(math.Mod(nan, 1)), math.Float64bits(math.Max(nan, 1)))
}

func decompose() {
	for _, x := range specials {
		frac, exp := math.Frexp(x)
		i, f := math.Modf(x)
		fmt.Println(x, frac, exp, math.Ldexp(frac, exp), i, f, math.Sqrt(x))
	}
	fmt.Println(math.Ldexp(1, 1024), math.Ldexp(1, -1075), math.Ldexp(1, -1074), math.Ldexp(0.75, -1073), math.Ldexp(1, 1<<40), math.Ldexp(1, -1<<40))
	fmt.Println(math.Sqrt(2), math.Sqrt(-1), math.Sqrt(math.Copysign(0, -1)), math.Sqrt(math.Inf(1)))
}

func cmpPackage() {
	fmt.Println(cmp.Compare(1, 2), cmp.Compare(2, 1), cmp.Compare(3, 3), cmp.Compare("a", "b"), cmp.Compare("b", "ab"))
	nan := math.NaN()
	fmt.Println(cmp.Compare(nan, 1.0), cmp.Compare(1.0, nan), cmp.Compare(nan, nan), cmp.Compare(0.0, math.Copysign(0, -1)))
	fmt.Println(cmp.Less(nan, 1.0), cmp.Less(1.0, nan), cmp.Less(nan, nan), cmp.Less(1, 2), cmp.Less("b", "a"))
	fmt.Println(cmp.Compare(Celsius(1.5), Celsius(-3)), cmp.Less(Name("x"), Name("y")), cmp.Compare(uint8(200), 100))
	fmt.Println(math.Signbit(cmp.Or(math.Copysign(0, -1), 1)), cmp.Or(nan, 2), cmp.Or(0.0, math.Copysign(0, -1)))
	fmt.Println(cmp.Or(0, 0, 3, 4), cmp.Or("", "x"), cmp.Or[string](), cmp.Or(Name(""), Name("n")) == "n")
}

func slicesPackage() {
	a := []int{1, 2, 3}
	fmt.Println(slices.Compare(a, []int{1, 2, 3}), slices.Compare(a, []int{1, 2}), slices.Compare(a, []int{1, 2, 3, 0}), slices.Compare(a, []int{1, 3}))
	fmt.Println(slices.Compare([]int(nil), []int{}), slices.Compare([]string{"b"}, []string{"a", "z"}))
	nan := math.NaN()
	fmt.Println(slices.Compare([]float64{nan}, []float64{nan}), slices.Compare([]float64{nan}, []float64{1}), slices.Equal([]float64{nan}, []float64{nan}))
	fmt.Println(slices.Equal(a, []int{1, 2, 3}), slices.Equal([]int(nil), []int{}), slices.Equal(a, a[:2]))
	fold := func(x, y string) bool { return strings.EqualFold(x, y) }
	fmt.Println(slices.EqualFunc([]string{"Go", "EO"}, []string{"go", "eo"}, fold), slices.EqualFunc([]string{"a"}, []string{"b"}, fold))
	byLen := func(x string, y int) int { return cmp.Compare(len(x), y) }
	five := func(x string, y int) int { return 5 * cmp.Compare(len(x), y) }
	fmt.Println(slices.CompareFunc([]string{"abc"}, []int{1}, five), slices.CompareFunc([]string{"a", "b"}, []int{1}, five))
	fmt.Println(slices.CompareFunc([]string{"ab", "c"}, []int{2, 1}, byLen), slices.CompareFunc([]string{"ab"}, []int{3}, byLen), slices.CompareFunc([]string{"ab"}, []int{2, 0}, byLen))
	for i := 0; i < 50; i++ {
		x := make([]int, pick(4))
		y := make([]int, pick(4))
		for j := range x {
			x[j] = pick(3)
		}
		for j := range y {
			y[j] = pick(3)
		}
		fmt.Print(slices.Compare(x, y), slices.Equal(x, y), " ")
	}
	fmt.Println()
}

func show(s string, bitSize int) {
	f, err := strconv.ParseFloat(s, bitSize)
	var ne *strconv.NumError
	if errors.As(err, &ne) {
		fmt.Printf("%q/%d: %v %x %v [%s %q %v range=%v syntax=%v]\n", s, bitSize, f, f, err, ne.Func, ne.Num, ne.Err, errors.Is(err, strconv.ErrRange), errors.Is(err, strconv.ErrSyntax))
		return
	}
	fmt.Printf("%q/%d: %v %x %v\n", s, bitSize, f, f, err)
}

func parseFloat() {
	inputs := []string{"", "0", "-0", "+0", "1", "-1.5", "+.5", "5.", ".", "-", "+", "e5", "1e", "1e+", "1e-5", "1E+05", "0.1", "1.7976931348623157e308",
		"1.7976931348623159e308", "1.797693134862315807e308", "-1e309", "1e-400", "-1e-400", "4.9e-324", "2.4703282292062327e-324",
		"2.4703282292062328e-324", "1e-320", "0e999999", "1e99999999999999999999", "0.000000000000000000000000000001e30",
		"123456789012345678901234567890", "3.4028234663852886e38", "3.4028235677973366e38", "3.4028235677973367e38", "1e-46", "1e-45",
		"inf", "Inf", "+INF", "-inf", "infinity", "-Infinity", "infin", "infinityx", "nan", "NaN", "-nan", "+nan", "nanx", "in",
		"0x1p0", "0x1.8p1", "-0x1P-2", "0x.8p1", "0x1", "0x1p", "0xp1", "0x", "0X1FFFFFFFFFFFFFp-52", "0x1.fffffffffffff8p1023", "0x1p-1074",
		"0x1p-1075", "0x1.8p-1075", "0x1p1024", "0x1.ffffffp127", "0x1.fffffefp127", "0x1_0p0", "0x_1p0", "1_000.5", "1__0", "0x1p1_0", "0x1p_10",
		"1.5x", "1.5 ", " 1.5", "1..5", "1.5.", "0.0.0", "1e5e5", "0b101", "0o17", "1e+-5", "+-1", "\u0661"}
	for _, s := range inputs {
		show(s, 64)
		show(s, 32)
	}
	show("0x1234567890abcdef12345p-10", 64)
	show("0x1234567890abcdef12345p-10", 32)
	show("1.5", 16)
	show(strings.Repeat("9", 400), 64)
	show("0."+strings.Repeat("0", 400)+"1", 64)
	show("1"+strings.Repeat("0", 330)+"e-330", 64)
}

func parseRounds() {
	digits := "0123456789"
	for i := 0; i < 300; i++ {
		var b strings.Builder
		if pick(3) == 0 {
			b.WriteByte('-')
		}
		n := 1 + pick(30)
		for j := 0; j < n; j++ {
			b.WriteByte(digits[pick(10)])
			if j == pick(n) && !strings.Contains(b.String(), ".") {
				b.WriteByte('.')
			}
		}
		b.WriteString("e")
		b.WriteString(strconv.Itoa(pick(700) - 360))
		s := b.String()
		f64, err64 := strconv.ParseFloat(s, 64)
		f32, err32 := strconv.ParseFloat(s, 32)
		fmt.Printf("%s %x %v %x %v\n", s, math.Float64bits(f64), err64 != nil, math.Float32bits(float32(f32)), err32 != nil)
	}
	for i := 0; i < 200; i++ {
		x := math.Float64frombits(next())
		if math.IsNaN(x) {
			continue
		}
		formats := []byte{'e', 'f', 'g', 'x'}
		format := formats[pick(4)]
		prec := pick(25) - 1
		if format == 'f' && math.Abs(x) > 1e30 {
			format = 'g'
		}
		s := strconv.FormatFloat(x, format, prec, 64)
		y, err := strconv.ParseFloat(s, 64)
		fmt.Printf("%x %s %x %v\n", math.Float64bits(x), s, math.Float64bits(y), err)
	}
	// The midpoints of adjacent float32s are float64s, so their exact decimals test ties to even,
	// and the decimals just above and below them test the rounding next to a tie.
	for i := 0; i < 150; i++ {
		lo := math.Float32frombits(uint32(next()>>33) & 0x7f7fffff)
		hi := math.Float32frombits(math.Float32bits(lo) + 1)
		mid := (float64(lo) + float64(hi)) / 2
		exact := strconv.FormatFloat(mid, 'e', 800, 64)
		mant, exp, _ := strings.Cut(exact, "e")
		mant = strings.TrimRight(mant, "0")
		tie := mant + "e" + exp
		above := mant + "1e" + exp
		f, _ := strconv.ParseFloat(tie, 32)
		g, _ := strconv.ParseFloat(above, 32)
		fmt.Printf("%x %x %x %x\n", math.Float32bits(lo), math.Float32bits(float32(f)), math.Float32bits(float32(g)), math.Float64bits(mid))
	}
}

func unicodePredicates() {
	type predicate struct {
		name string
		is   func(rune) bool
	}
	predicates := []predicate{{"IsControl", unicode.IsControl}, {"IsLetter", unicode.IsLetter}, {"IsUpper", unicode.IsUpper},
		{"IsLower", unicode.IsLower}, {"IsTitle", unicode.IsTitle}, {"IsMark", unicode.IsMark}, {"IsNumber", unicode.IsNumber},
		{"IsDigit", unicode.IsDigit}, {"IsPunct", unicode.IsPunct}, {"IsSymbol", unicode.IsSymbol}, {"IsGraphic", unicode.IsGraphic},
		{"IsPrint", unicode.IsPrint}, {"IsSpace", unicode.IsSpace}}
	for _, p := range predicates {
		count := 0
		hash := uint64(14695981039346656037)
		for r := rune(-2); r <= unicode.MaxRune+2; r++ {
			if p.is(r) {
				count++
				hash = (hash ^ uint64(r)) * 1099511628211
			}
		}
		fmt.Printf("%s %d %x\n", p.name, count, hash)
	}
	for _, r := range []rune{'a', 'Z', '5', '_', ' ', '\u00a0', '\u00aa', '\u00b2', '\u00bd', '\u01c5', '\u0300', '\u0660', '\u2028', '\u3000', '\U0001F600', -1, unicode.MaxRune + 1, math.MaxInt32, math.MinInt32} {
		fmt.Printf("%U", r)
		for _, p := range predicates {
			if p.is(r) {
				fmt.Print(" ", p.name)
			}
		}
		fmt.Println()
	}
}

func main() {
	constants()
	bitsAndSigns()
	rounding()
	minMax()
	decompose()
	cmpPackage()
	slicesPackage()
	parseFloat()
	parseRounds()
	unicodePredicates()
}
