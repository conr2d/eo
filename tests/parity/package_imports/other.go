package main

import (
	"github.com/conr2d/eo/tests/parity/package_imports/a"
	"github.com/conr2d/eo/tests/parity/package_imports/trace"
	util "github.com/conr2d/eo/tests/parity/package_imports/u2/util"
)

// Holder embeds a type of another package. util here is not the util of main.go.
type Holder struct {
	a.Point
	Box   util.Box
	trace string
}

// Record names the package trace while Holder has a field trace.
func (h Holder) Record() {
	trace.Add("record " + h.trace)
}

type Square struct {
	Side int
}

func (s Square) Area() int {
	return s.Side * s.Side
}

func box(label string) util.Box {
	return util.Box{Label: label}
}

func other() string {
	return util.Name()
}
