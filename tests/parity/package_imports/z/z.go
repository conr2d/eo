package z

import "github.com/conr2d/eo/tests/parity/package_imports/trace"

var Z = trace.Mark("z.Z")

func init() {
	trace.Add("z.init")
}

// Relay sends n on a new channel from a goroutine and returns what it receives, so it blocks.
func Relay(n int) int {
	ch := make(chan int)
	go func() { ch <- n }()
	return <-ch
}
