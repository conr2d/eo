package main

import (
	"encoding/json"
	"fmt"
	"math/big"
)

// Point has a String method, which a defined type over it does not inherit.
type Point struct {
	X, Y int
}

func (p Point) String() string {
	return fmt.Sprintf("(%d, %d)", p.X, p.Y)
}

func (p *Point) Move(dx int) {
	p.X += dx
}

// Vec is Point's struct type without Point's methods.
type Vec Point

// Pair is a defined type over Vec, so over Point's struct type too, with a method of its own.
type Pair Vec

func (p *Pair) Swap() {
	x := p.X
	p.X = p.Y
	p.Y = x
}

type Holder struct {
	V Vec
	P Pair
}

type Tagged struct {
	Name string `json:"name"`
	N    int    `json:"n,omitempty"`
}

type Alias Tagged

// Amount is big.Int with methods of its own, as go-ethereum's hexutil.Big is.
type Amount big.Int

func (a *Amount) String() string {
	return "amount " + (*big.Int)(a).String()
}

func (a Amount) MarshalText() ([]byte, error) {
	return []byte((*big.Int)(&a).Text(16)), nil
}

type Celsius float64

type Hash [2]uint8

type Key Hash

func main() {
	p := Point{1, 2}
	v := Vec(p)
	fmt.Println(p, v, Point(v), v.X)
	fmt.Printf("%v %+v %T %T\n", v, v, v, p)
	_, isStringer := any(v).(fmt.Stringer)
	fmt.Println(isStringer, v == Vec{1, 2}, Point(v) == p)

	pv := (*Point)(&v)
	pv.Move(10)
	fmt.Println(v, *pv, pv.String())
	pp := &p
	vp := (*Vec)(pp)
	vp.Y = 7
	fmt.Println(p, *vp)

	pair := Pair(v)
	pair.Swap()
	fmt.Println(pair, Point(pair), Vec(pair))
	(*Pair)(pp).Swap()
	fmt.Println(p)

	fmt.Printf("%#v %v %+v\n", v, []Vec{v, {3, 4}}, Holder{v, pair})
	w := v
	q := pair
	fmt.Printf("%v %+v %#v\n", &w, &q, &w)
	seen := map[Vec]string{v: "v"}
	fmt.Println(seen[Vec(p)], seen[Vec{11, 2}], len(seen))
	switch x := any(pair).(type) {
	case Point:
		fmt.Println("point", x)
	case Pair:
		fmt.Println("pair", x)
	}

	var none *Point
	fmt.Println((*Vec)(none) == nil, (*Pair)((*Vec)(none)) == nil)

	a := Alias{Name: "eo"}
	out, err := json.Marshal(a)
	fmt.Println(string(out), err)
	var back Alias
	err = json.Unmarshal([]byte(`{"name":"go","n":3}`), &back)
	fmt.Println(back, err, Tagged(back))

	n := big.NewInt(255)
	amount := (*Amount)(n)
	fmt.Println(amount, amount.String())
	n.SetInt64(4096)
	fmt.Println(amount)
	(*big.Int)(amount).Neg(n)
	fmt.Println(n, amount)
	fmt.Println(Amount(*n), *amount)
	text, _ := Amount(*n).MarshalText()
	fmt.Println(string(text))
	var zero Amount
	text, err = zero.MarshalText()
	fmt.Println((*big.Int)(&zero).Sign(), string(text), err)
	enc, err := json.Marshal(map[string]*Amount{"v": amount})
	fmt.Println(string(enc), err)

	f := 36.6
	c := (*Celsius)(&f)
	*c += 1
	fmt.Printf("%.1f %.1f %T\n", f, *c, c)
	h := Hash{1, 2}
	k := (*Key)(&h)
	k[0] = 9
	fmt.Println(h, *k, Hash(*k) == h)
	hs := (*[2]uint8)(k)
	hs[1] = 8
	fmt.Println(h)
}
