package main

import (
	"errors"
	"fmt"
	"strconv"
)

type intCase struct {
	s       string
	base    int
	bitSize int
}

func parseInt(c intCase) {
	i, err := strconv.ParseInt(c.s, c.base, c.bitSize)
	u, uerr := strconv.ParseUint(c.s, c.base, c.bitSize)
	fmt.Println(c.s, c.base, c.bitSize, i, err, u, uerr)
}

func atoi(s string) {
	n, err := strconv.Atoi(s)
	isRange := errors.Is(err, strconv.ErrRange)
	isSyntax := errors.Is(err, strconv.ErrSyntax)
	fmt.Println(s, n, err, isRange, isSyntax)
}

func unquote(s string) {
	u, err := strconv.Unquote(s)
	fmt.Printf("%s %q %v\n", s, u, err)
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	// ParseInt and ParseUint, with prefixes, underscores, signs, bit sizes, and range errors.
	for _, c := range []intCase{
		{"42", 10, 64}, {"-42", 10, 64}, {"+7", 10, 0}, {"0x_1F", 0, 64}, {"0b101", 0, 8}, {"0o17", 0, 64},
		{"017", 0, 64}, {"0", 0, 64}, {"1_000", 0, 64}, {"1__0", 0, 64}, {"_1", 0, 64}, {"1_000", 10, 64},
		{"ff", 16, 64}, {"FF", 16, 8}, {"zz", 36, 64}, {"z", 37, 64}, {"1", 1, 64}, {"1", 10, 65}, {"1", 10, -1},
		{"", 10, 64}, {"-", 10, 64}, {"12a", 10, 64}, {"128", 10, 8}, {"-128", 10, 8}, {"-129", 10, 8},
		{"255", 10, 8}, {"256", 10, 8}, {"9223372036854775807", 10, 64}, {"9223372036854775808", 10, 64},
		{"-9223372036854775808", 10, 64}, {"-9223372036854775809", 10, 64}, {"18446744073709551615", 10, 64},
		{"18446744073709551616", 10, 64}, {"99999999999999999999999", 10, 64}, {"0x", 0, 64}, {"0x8000000000000000", 0, 64},
	} {
		parseInt(c)
	}
	for _, s := range []string{"123", "-123", "+0", "", "+", "-", "1x", "12345678901234567890", "-9223372036854775808", "1_0"} {
		atoi(s)
	}
	for _, s := range []string{"1", "t", "T", "true", "TRUE", "True", "0", "f", "false", "FALSE", "yes", ""} {
		b, err := strconv.ParseBool(s)
		fmt.Printf("%v %v; ", b, err)
	}
	fmt.Println()

	// Formatting and appending.
	minInt := int64(-1)
	minInt <<= 63
	fmt.Println(strconv.Itoa(-45), strconv.FormatInt(-255, 16), strconv.FormatInt(minInt, 2), strconv.FormatUint(18446744073709551615, 36), strconv.FormatUint(0, 7), strconv.FormatBool(true))
	buf := []byte("x=")
	buf = strconv.AppendInt(buf, -12, 10)
	buf = strconv.AppendUint(buf, 255, 16)
	buf = strconv.AppendBool(buf, false)
	buf = strconv.AppendQuote(buf, "a\"b")
	fmt.Println(string(buf), strconv.IntSize)

	// Quote, QuoteRune, IsPrint, and Unquote.
	fmt.Println(strconv.Quote("hi\n\t\"\\\x00\x7f\xff é ☺ \u2028"), strconv.QuoteRune('☺'), strconv.QuoteRune('\''), strconv.QuoteRune('\n'), strconv.QuoteRune(-1))
	fmt.Println(strconv.IsPrint('a'), strconv.IsPrint('\t'), strconv.IsPrint('é'), strconv.IsPrint(0xad), strconv.IsPrint(0x2028), strconv.IsPrint(-1))
	for _, s := range []string{`"abc"`, `"a\nb"`, `"\x41\u00e9\U0001F600\101"`, `'a'`, `'\''`, `'ab'`, "`raw\\n\r`", `"unterminated`, `"bad\q"`, `"a"b`, `'\"'`, `"\'"`, `""`, `''`, `"\400"`, `"\uD800"`, `x`, `"é"`, "\"\xff\""} {
		unquote(s)
	}
	r, multibyte, tail, err := strconv.UnquoteChar(`\u263a rest`, '"')
	fmt.Println(r, multibyte, tail, err)

	// A NumError unwraps to its reason, and errors.As finds it.
	_, err = strconv.ParseInt("999", 10, 8)
	var ne *strconv.NumError
	if errors.As(err, &ne) {
		fmt.Println(ne.Func, ne.Num, ne.Err, ne.Err == strconv.ErrRange)
	}
	fmt.Printf("%v %T\n", err, err)

	attempt("base", func() { strconv.FormatInt(1, 1) })
	attempt("append base", func() { strconv.AppendUint(nil, 1, 37) })
}
