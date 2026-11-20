package main

import "fmt"

// Constants used by name.
const Greeting = "hello"

const (
	KB = 1 << (10 * (iota + 1))
	MB
	GB
)

const (
	A, B = iota, iota * 10
	_, _
	C, D
)

const Pi = 3.14159265358979323846264338327950288
const MaxU8 uint8 = 255
const Third float32 = 1.0 / 3
const debug = false
const Letter = 'x'
const Ratio = 7 / 2 * 2.0

const Big = 1 << 100
const Huge = 1e400

const unusedPkg = 7

type Box struct {
	KB int64
}

func (b Box) Scale() int64 {
	return b.KB * KB
}

func area(r float64) float64 {
	return Pi * r * r
}

func label(n int64) string {
	switch n {
	case KB:
		return "KB"
	case MB:
		return "MB"
	}
	return "?"
}

func main() {
	fmt.Println(Greeting, KB, MB, GB)
	fmt.Println(A, B, C, D)
	fmt.Println(Big>>98, Big/GB/GB/GB, Huge/1e399)

	var f32 float32 = 2
	fmt.Println(f32*Pi, area(2), Pi)
	fmt.Println(MaxU8, MaxU8/2, Third, Third*3, Ratio)

	var u uint8 = 200
	fmt.Println(u+MaxU8, u == MaxU8, u < MaxU8)

	if debug {
		fmt.Println("debug")
	}
	r := Letter
	var b byte = 'x'
	fmt.Println(r, b == Letter, string(rune(Letter)))

	const local = KB * 3 / 2
	const unused = 1
	x := local
	var y int32 = local
	var z float64 = local
	fmt.Println(x, y, z, local/7)

	n := 5
	fmt.Println(n*KB, n<<A, KB<<n, n/MB)

	s := Greeting + ", world"
	t := Greeting
	t += "!"
	fmt.Println(s, t, len(Greeting), Greeting == t)

	fmt.Println(Box{2}.Scale(), label(1024), label(MB), label(GB))

	{
		const KB = "kilo"
		fmt.Println(KB)
	}
	for i := 0; i < 2; i++ {
		const i = 10
		fmt.Println(i)
	}

	func() {
		fmt.Println(local, Greeting)
	}()

	defer fmt.Println("deferred", KB, Greeting)
	done := make(chan bool)
	go func() {
		fmt.Println("goroutine", local)
		done <- true
	}()
	<-done
}
