package main

import (
	"fmt"

	"github.com/conr2d/eo/tests/parity/package_imports/a"
	_ "github.com/conr2d/eo/tests/parity/package_imports/m"
	"github.com/conr2d/eo/tests/parity/package_imports/trace"
	"github.com/conr2d/eo/tests/parity/package_imports/u1/util"
)

func main() {
	// Go initializes the first package, in import path order, whose imports are initialized:
	// trace, m, z, a. A depth-first walk of the imports would give trace, z, a, m.
	for _, e := range trace.Events {
		fmt.Println(e)
	}
	fmt.Println(a.A)
	fmt.Println(util.Name(), other())

	h := Holder{Point: a.Point{X: 1, Y: 2}, Box: box("b")}
	h.Scale(3)
	fmt.Println(h.Sum(), h.Point, h.Box.Label)
	fmt.Printf("%T %T\n", h.Point, h)

	var s a.Shape = Square{Side: 4}
	fmt.Println(s.Area(), a.Doubled(21))

	h.trace = "x"
	h.Record()
	fmt.Println(trace.Events[len(trace.Events)-1])
}
