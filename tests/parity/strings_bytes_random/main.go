package main

import (
	"bytes"
	"fmt"
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
	return int(next() >> 33 % uint64(n))
}

var alphabet = []string{"a", "b", "B", "k", " ", "\t", "\n", "é", "É", "ß", "\u212a", "\xff", "\xc3", "☺", ",", "_", "Σ", "ς", "\u00a0", "ab"}

var runes = []rune{'a', 0xE9, 0xFFFD, -1, 0x212A, 0x263A, ','}

func randString() string {
	var b strings.Builder
	k := pick(8)
	for i := 0; i < k; i++ {
		b.WriteString(alphabet[pick(len(alphabet))])
	}
	return b.String()
}

func isComma(r rune) bool {
	return r == ',' || r == '_'
}

func dropA(r rune) rune {
	if r == 'a' {
		return -1
	}
	return unicode.ToUpper(r)
}

func show(b []byte) string {
	if b == nil {
		return "nil"
	}
	return fmt.Sprintf("%q", b)
}

func showAll(bs [][]byte) string {
	if bs == nil {
		return "nil"
	}
	var b strings.Builder
	for _, x := range bs {
		b.WriteString(show(x))
		b.WriteString(" ")
	}
	return b.String()
}

func stringOps(s, t, u string, n int) {
	i1 := strings.Index(s, t)
	i2 := strings.LastIndex(s, t)
	c := strings.Count(s, t)
	has := strings.Contains(s, t)
	pre := strings.HasPrefix(s, t)
	suf := strings.HasSuffix(s, t)
	ia := strings.IndexAny(s, u)
	ca := strings.ContainsAny(s, u)
	cmp := strings.Compare(s, t)
	fmt.Printf("%d %d %d %v %v %v %d %v %d\n", i1, i2, c, has, pre, suf, ia, ca, cmp)
	split := strings.Split(s, t)
	splitN := strings.SplitN(s, t, n)
	fields := strings.Fields(s)
	ff := strings.FieldsFunc(s, isComma)
	before, after, found := strings.Cut(s, t)
	fmt.Printf("%q %q %q %q %q %q %v %v\n", split, splitN, fields, ff, before, after, found, splitN == nil)
	tr := strings.Trim(s, u)
	tl := strings.TrimLeft(s, u)
	trr := strings.TrimRight(s, u)
	ts := strings.TrimSpace(s)
	tp := strings.TrimPrefix(s, t)
	tsf := strings.TrimSuffix(s, t)
	tf := strings.TrimFunc(s, isComma)
	fmt.Printf("%q %q %q %q %q %q %q\n", tr, tl, trr, ts, tp, tsf, tf)
	up := strings.ToUpper(s)
	lo := strings.ToLower(s)
	ef1 := strings.EqualFold(s, up)
	ef2 := strings.EqualFold(lo, up)
	ef3 := strings.EqualFold(s, t)
	rep := strings.Replace(s, t, u, n)
	ra := strings.ReplaceAll(s, t, u)
	m := strings.Map(dropA, s)
	j := strings.Join(split, u)
	r := strings.Repeat(t, n+2)
	fmt.Printf("%q %q %v %v %v %q %q %q %q %q\n", up, lo, ef1, ef2, ef3, rep, ra, m, j, r)
	rr := runes[pick(len(runes))]
	bb := byte(pick(256))
	ir := strings.IndexRune(s, rr)
	cr := strings.ContainsRune(s, rr)
	ib := strings.IndexByte(s, bb)
	lib := strings.LastIndexByte(s, bb)
	rp := strings.NewReplacer(t, u, "a", "A", "", "_", "a", "X").Replace(s)
	rp2 := strings.NewReplacer("a", "1", "b", "22", t, "").Replace(s)
	fmt.Printf("%d %v %d %d %q %q\n", ir, cr, ib, lib, rp, rp2)
}

func bytesOps(s, t []byte, u string, n int) {
	eq := bytes.Equal(s, t)
	cmp := bytes.Compare(s, t)
	pre := bytes.HasPrefix(s, t)
	suf := bytes.HasSuffix(s, t)
	has := bytes.Contains(s, t)
	i1 := bytes.Index(s, t)
	i2 := bytes.LastIndex(s, t)
	c := bytes.Count(s, t)
	ia := bytes.IndexAny(s, u)
	ib := bytes.IndexByte(s, 'a')
	fmt.Printf("%v %d %v %v %v %d %d %d %d %d\n", eq, cmp, pre, suf, has, i1, i2, c, ia, ib)
	split := bytes.Split(s, t)
	splitN := bytes.SplitN(s, t, n)
	before, after, found := bytes.Cut(s, t)
	fmt.Println(showAll(split), "|", showAll(splitN), "|", show(before), show(after), found)
	tr := bytes.Trim(s, u)
	tl := bytes.TrimLeft(s, u)
	trr := bytes.TrimRight(s, u)
	ts := bytes.TrimSpace(s)
	tp := bytes.TrimPrefix(s, t)
	tsf := bytes.TrimSuffix(s, t)
	tf := bytes.TrimFunc(s, isComma)
	fmt.Println(show(tr), show(tl), show(trr), show(ts), show(tp), show(tsf), show(tf))
	cl := bytes.Clone(s)
	up := bytes.ToUpper(s)
	lo := bytes.ToLower(s)
	ef := bytes.EqualFold(s, up)
	rep := bytes.Replace(s, t, []byte(u), n)
	ra := bytes.ReplaceAll(s, t, []byte(u))
	m := bytes.Map(dropA, s)
	j := bytes.Join(split, []byte(u))
	r := bytes.Repeat(t, n+2)
	fmt.Println(show(cl), show(up), show(lo), ef, show(rep), show(ra), show(m), show(j), show(r))

	// The results that are slices of s share its bytes.
	x := []byte(string(s))
	b2, a2, _ := bytes.Cut(x, t)
	if len(b2) > 0 {
		b2[0] = '@'
	}
	if len(a2) > 0 {
		a2[len(a2)-1] = '#'
	}
	parts := bytes.Split(x, t)
	if len(parts) > 1 && len(parts[0]) > 0 {
		parts[0][0] = '%'
	}
	trimmed := bytes.TrimRight(x, u)
	if len(trimmed) > 0 {
		trimmed[len(trimmed)-1] = '&'
	}
	fmt.Println(show(x))
}

func main() {
	for i := 0; i < 400; i++ {
		s := randString()
		t := randString()
		u := randString()
		n := pick(5) - 2
		stringOps(s, t, u, n)
		var bs []byte
		if pick(4) != 0 {
			bs = []byte(s)
		}
		var bt []byte
		if pick(4) != 0 {
			bt = []byte(t)
		}
		bytesOps(bs, bt, u, n)
	}
}
