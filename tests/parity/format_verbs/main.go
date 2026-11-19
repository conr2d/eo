package main

import (
	"errors"
	"fmt"
)

type Pair struct {
	A int
	S string
}

type Name struct {
	N int
}

func (n Name) String() string {
	return fmt.Sprintf("name-%d", n.N)
}

func main() {
	fmt.Printf("%d %s %v %x %X %q %t %T %%\n", 42, "s", Pair{1, "a"}, 255, "hi", "q\"\n\x01é", true, Pair{})
	fmt.Printf("%v %d %x %o %b %c %q %U\n", -7, int8(-3), uint8(200), 8, 5, 'é', 'a', 'é')
	fmt.Printf("%v %g %e %f %v\n", 1.5, 1e21, 1234.5678, 0.1, float32(0.1))
	fmt.Printf("%d|%s|%x|%q|%v|%s\n", []int{1, 2}, []string{"a", "b"}, []byte("hi"), []byte("hi"), []byte("hi"), []byte(nil))
	fmt.Printf("%d|%s|%x|%v|%v\n", Pair{1, "a"}, Pair{1, "a"}, Pair{255, "a"}, &Pair{1, "a"}, []*Pair{nil})
	fmt.Printf("%v|%s|%d|%q|%x\n", Name{1}, Name{2}, Name{3}, Name{4}, Name{5})
	fmt.Printf("%d|%s|%z|%d\n", "s", 3, 1)
	fmt.Printf("%d", 1, "x", 2.5)
	fmt.Println()
	fmt.Printf("%v|%d|%s|%T|%w\n", nil, nil, error(nil), nil, errors.New("x"))
	fmt.Printf("%x|%X|%x|%q\n", -255, "hi", "", "\xffa\u00ad\t")
	fmt.Printf("%t|%v|%s\n", 1, 1.5, 2.5)
	fmt.Printf("trail%")
	fmt.Println()
	s := fmt.Sprintf("%v and %x", []any{1, "a", nil}, []int{10, 11})
	fmt.Println(s, len(s))
	fmt.Println(fmt.Sprint("a", "b", 1, 2, "c"), fmt.Sprint(1, 2), fmt.Sprintln("x", 3) == "x 3\n")
}
