package main

import (
	"errors"
	"fmt"
)

// Celsius is a temperature, with a String method.
type Celsius float64

type Fahrenheit float64

func (c Celsius) ToF() Fahrenheit {
	return Fahrenheit(c*9/5 + 32)
}

func (c Celsius) String() string {
	return fmt.Sprintf("%gC", float64(c))
}

// Weekday is an enumeration.
type Weekday int

const (
	Sunday Weekday = iota
	Monday
	Tuesday
)

const Default Name = "anon"

func (d Weekday) String() string {
	switch d {
	case Sunday:
		return "Sun"
	case Monday:
		return "Mon"
	case Tuesday:
		return "Tue"
	}
	return fmt.Sprintf("Weekday(%d)", int(d))
}

func (d Weekday) Next() Weekday {
	return (d + 1) % 7
}

type Prio int8

type Counter uint32

func (c *Counter) Inc() {
	*c++
}

type Flag bool

type Name string

func (n Name) Greet() string {
	return "hi " + string(n)
}

type Data []byte

func (d Data) Len() int {
	return len(d)
}

type Set map[string]bool

func (s Set) Has(k string) bool {
	return s[k]
}

type Op func(int) int

func (f Op) Twice(x int) int {
	return f(f(x))
}

type Kelvin Celsius

type Code uint8

func (c Code) Error() string {
	return fmt.Sprintf("code %d", uint8(c))
}

type Reading struct {
	T   Celsius
	Day Weekday
}

func main() {
	c := Celsius(37.5)
	fmt.Println(c, c.ToF(), float64(c), c > 30, c+0.5)
	f := c.ToF()
	fmt.Println(f, Celsius(f-32)*5/9, int(c))
	fmt.Printf("%v %g %T %T\n", c, c, c, f)

	d := Tuesday
	next := Weekday.Next
	fmt.Println(d, Monday, d.Next(), next(Weekday(6)), Weekday(9), int(d))
	fmt.Printf("%v %d %s %T\n", d, d, d, d)

	var p Prio = 127
	p++
	var minus Prio = -1
	fmt.Println(p, p/minus, p%minus, -p, p>>1, int8(p))

	var n Counter
	n.Inc()
	n.Inc()
	pc := &n
	pc.Inc()
	var k uint = 33
	fmt.Println(n, n^0xff, n&^1, n%2, n<<k, Counter(1)<<4)

	ok := Flag(true)
	fmt.Println(ok, !ok, ok == true)
	fmt.Printf("%T %s\n", ok, ok)

	nm := Name("eo")
	nm += "!"
	fmt.Println(nm, nm.Greet(), len(nm), nm[1:], nm < "z", nm+Name("?"), Default)
	fmt.Printf("%q %T %d\n", nm, nm, nm)

	data := Data{1, 2}
	data = append(data, 10)
	fmt.Println(data, data.Len(), data[1:].Len())
	fmt.Printf("%x %T %v\n", data, data, &data)
	for i, b := range data {
		fmt.Println(i, b)
	}
	var none Data
	fmt.Println(none == nil, len(none), none)

	set := Set{"a": true}
	set["b"] = true
	fmt.Println(set, set.Has("b"), set.Has("z"), len(set))

	var op Op = func(x int) int { return x * 3 }
	fmt.Println(op(2), op.Twice(2), Op(nil) == nil)

	var err error = Code(4)
	code, isCode := err.(Code)
	fmt.Println(err, code, isCode, errors.Is(err, Code(4)))

	temps := map[Weekday]Celsius{Monday: 20, Sunday: 18.5}
	fmt.Println(temps, temps[Monday])

	r := Reading{T: 1, Day: Monday}
	fmt.Println(r, r == Reading{1, Monday})

	var s fmt.Stringer = d
	fmt.Println(s, s.(Weekday) == Tuesday)
	switch v := any(c).(type) {
	case Weekday:
		fmt.Println("weekday", v)
	case Celsius:
		fmt.Println("celsius", v)
	}

	days := []Weekday{Sunday, 2}
	fmt.Println(days, len(days))

	size := Tuesday + 1
	names := make([]string, size)
	names[Monday] = "mon"
	both := ok && !ok
	steps := Weekday(20)
	steps /= d
	raw := []byte("raw")
	kv := Kelvin(c) + 273.15
	fmt.Println(names, len(names), both, steps, Name(raw), nm < Default, kv)
	fmt.Printf("%T %v\n", kv, Celsius(kv))

	letter := Weekday(0x4f)
	keys := 0
	for k := range set {
		keys += len(k)
	}
	for i, ch := range Name("é!") {
		fmt.Println(i, ch)
	}
	fmt.Println(string(rune(letter)), string(letter), keys)
}
