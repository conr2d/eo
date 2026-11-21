package a

import (
	"github.com/conr2d/eo/tests/parity/package_imports/trace"
	"github.com/conr2d/eo/tests/parity/package_imports/z"
)

var A = trace.Mark("a.A after " + z.Z)

func init() {
	trace.Add("a.init")
}

type Point struct {
	X, Y int
}

func (p Point) Sum() int {
	return p.X + p.Y
}

func (p *Point) Scale(k int) {
	p.X *= k
	p.Y *= k
}

type Shape interface {
	Area() int
}

// Doubled calls a function of z that blocks.
func Doubled(n int) int {
	return 2 * z.Relay(n)
}
