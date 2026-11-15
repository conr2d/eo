package main

import "fmt"

func sum(label string, xs ...int) {
	total := 0
	for i := 0; i < len(xs); i++ {
		total += xs[i]
	}
	if len(xs) > 0 {
		xs[0] = -xs[0]
	}
	fmt.Println(label, total, len(xs), xs == nil)
}

func main() {
	a := []int{1, 2, 3}
	b := a
	b[0] = 9
	fmt.Println(a, b)

	t := a[1:2]
	fmt.Println(len(t), cap(t))
	t = append(t, 7)
	fmt.Println(a, t)
	u := append(t, 8)
	u[0] = 100
	fmt.Println(a, t, u)

	limited := a[0:1:1]
	limited = append(limited, -1)
	fmt.Println(a, limited, cap(a[1:2:2]))

	var none []int
	empty := []int{}
	fmt.Println(none == nil, empty == nil, len(none), cap(none), none[:0] == nil, empty[:0] == nil)
	fmt.Println(append(none, none...) == nil, append(empty, none...) == nil, none, empty)

	m := make([]int, 2, 5)
	fmt.Println(m, len(m), cap(m), m[:5])
	grown := append(m, 1, 2, 3)
	grown[0] = 4
	fmt.Println(m, grown, m[:5])

	c := []int{1, 2, 3, 4, 5}
	n := copy(c[1:], c)
	fmt.Println(n, c)
	c = append(c[:1], c[2:]...)
	fmt.Println(c, len(c), cap(c))
	clear(c[1:3])
	fmt.Println(c)

	words := [][]string{{"a", "b"}, nil, {}}
	words[1] = append(words[1], "c")
	fmt.Println(words, len(words[2]), words[2] == nil, words[1][0])

	bs := []byte("héllo")
	bs[0] = 'H'
	bs = append(bs, " world"...)
	fmt.Println(bs[:3], string(bs))
	n = copy(bs, "J")
	fmt.Println(n, string(bs[:5]))
	rs := []rune("héllo\xff")
	fmt.Println(rs, len(rs), string(rs[1:3]), string(rs))

	sum("none")
	sum("two", 1, 2)
	args := []int{3, 4, 5}
	sum("spread", args...)
	fmt.Println(args)
}
