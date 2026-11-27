// The module says go 1.22, whose GODEBUG would let crypto functions read the reader they are given.
// Eo maps the default of Go 1.26, which ignores it.
//go:debug cryptocustomrand=0

package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"errors"
	"fmt"
	"io"
	"math/big"
	"reflect"
)

// toyCurve is not a real curve: its base point multiple k*G is (k mod P, 2k mod P), which is enough
// to see that GenerateKey derives the public key from D.
type toyCurve struct {
	params *elliptic.CurveParams
	calls  int
	reads  int
}

func (c *toyCurve) Params() *elliptic.CurveParams {
	c.reads++
	return c.params
}

func (c *toyCurve) IsOnCurve(x, y *big.Int) bool {
	twice := new(big.Int).Lsh(x, 1)
	return twice.Mod(twice, c.params.P).Cmp(y) == 0
}

func (c *toyCurve) Add(x1, y1, x2, y2 *big.Int) (*big.Int, *big.Int) {
	x := new(big.Int).Add(x1, x2)
	y := new(big.Int).Add(y1, y2)
	return x.Mod(x, c.params.P), y.Mod(y, c.params.P)
}

func (c *toyCurve) Double(x1, y1 *big.Int) (*big.Int, *big.Int) {
	return c.Add(x1, y1, x1, y1)
}

func (c *toyCurve) ScalarMult(x1, y1 *big.Int, k []byte) (*big.Int, *big.Int) {
	s := new(big.Int).SetBytes(k)
	x := new(big.Int).Mul(x1, s)
	y := new(big.Int).Mul(y1, s)
	return x.Mod(x, c.params.P), y.Mod(y, c.params.P)
}

func (c *toyCurve) ScalarBaseMult(k []byte) (*big.Int, *big.Int) {
	c.calls++
	return c.ScalarMult(c.params.Gx, c.params.Gy, k)
}

// countReader returns bytes 1, 2, 3, ... and counts the calls of Read.
type countReader struct {
	next  byte
	reads int
}

func (r *countReader) Read(p []byte) (int, error) {
	r.reads++
	for i := range p {
		r.next++
		p[i] = r.next
	}
	return len(p), nil
}

// failReader fails every Read.
type failReader struct{}

func (failReader) Read(p []byte) (int, error) { return 0, errors.New("no entropy") }

func newToyCurve(bits int) *toyCurve {
	p := new(big.Int).Lsh(big.NewInt(1), uint(bits))
	p.Sub(p, big.NewInt(1))
	n := new(big.Int).Sub(p, big.NewInt(2))
	return &toyCurve{params: &elliptic.CurveParams{
		P:       p,
		N:       n,
		B:       big.NewInt(7),
		Gx:      big.NewInt(1),
		Gy:      big.NewInt(2),
		BitSize: bits,
		Name:    "toy",
	}}
}

func randomBytes() {
	fmt.Printf("%T\n", rand.Reader)
	b := make([]byte, 64)
	n, err := rand.Read(b)
	fmt.Println(n, err)
	zero := 0
	for _, x := range b {
		if x == 0 {
			zero++
		}
	}
	// 64 random bytes have more than 16 zeros with negligible probability.
	fmt.Println(zero <= 16)

	n, err = rand.Read(nil)
	fmt.Println(n, err)

	var q [16]byte
	n, err = io.ReadFull(rand.Reader, q[:])
	fmt.Println(n, err, q != [16]byte{})

	// A replaced Reader is read by Read.
	saved := rand.Reader
	cr := &countReader{}
	rand.Reader = cr
	b = make([]byte, 5)
	n, err = rand.Read(b)
	fmt.Println(n, err, b, cr.reads)
	rand.Reader = saved
}

func curveParams() {
	c := newToyCurve(61)
	p := c.Params()
	fmt.Println(p.Params() == p, p.Name, p.BitSize, p.P, p.N, p.B, p.Gx, p.Gy)

	var curve elliptic.Curve = c
	x, y := curve.ScalarBaseMult([]byte{3})
	fmt.Println(x, y, curve.IsOnCurve(x, y), curve.IsOnCurve(y, x))
	x, y = curve.Double(x, y)
	fmt.Println(x, y)
	x, y = curve.Add(x, y, big.NewInt(1), big.NewInt(2))
	fmt.Println(x, y)
	x, y = curve.ScalarMult(x, y, []byte{1, 0})
	fmt.Println(x, y)
}

func keys() {
	for _, bits := range []int{5, 8, 61, 256} {
		c := newToyCurve(bits)
		cr := &countReader{}
		priv, err := ecdsa.GenerateKey(c, cr)
		if err != nil {
			fmt.Println(err)
			continue
		}
		d := priv.D
		inRange := d.Sign() > 0 && d.Cmp(c.params.N) < 0
		wantX, wantY := c.ScalarMult(c.params.Gx, c.params.Gy, d.Bytes())
		fmt.Println(bits, inRange, priv.X.Cmp(wantX) == 0, priv.Y.Cmp(wantY) == 0)
		// The reader passed in is ignored, ScalarBaseMult is called once, before the check above, and
		// Params is called once to pick the curve's implementation and once for each draw of D, which
		// is a single draw for a large N but for a negligible chance.
		fmt.Println(cr.reads, c.calls, c.reads >= 2, bits < 61 || c.reads == 2)
		fmt.Println(priv.PublicKey.Curve == elliptic.Curve(c), priv.Curve.Params().Name, priv.Params().BitSize)
		fmt.Println(priv.IsOnCurve(priv.X, priv.Y))
	}

	// A failing reader is ignored too.
	priv, err := ecdsa.GenerateKey(newToyCurve(16), failReader{})
	fmt.Println(priv != nil, err)

	// So is a replaced Reader.
	saved := rand.Reader
	cr := &countReader{}
	rand.Reader = cr
	priv, err = ecdsa.GenerateKey(newToyCurve(16), rand.Reader)
	fmt.Println(priv != nil, err, cr.reads)
	rand.Reader = saved

	// Two keys differ.
	c := newToyCurve(256)
	a, _ := ecdsa.GenerateKey(c, rand.Reader)
	b, _ := ecdsa.GenerateKey(c, rand.Reader)
	fmt.Println(a.D.Cmp(b.D) != 0)

	pub := a.PublicKey
	same := ecdsa.PublicKey{Curve: c, X: a.X, Y: a.Y}
	other := ecdsa.PublicKey{Curve: c, X: b.X, Y: b.Y}
	fmt.Println(pub == same, pub == other)
	fmt.Println(*a == *a, *a == *b)
	var zero ecdsa.PrivateKey
	fmt.Println(zero.Curve == nil, zero.X == nil, zero.D == nil)
}

func types() {
	for _, t := range []reflect.Type{reflect.TypeOf(ecdsa.PublicKey{}), reflect.TypeOf(ecdsa.PrivateKey{}),
		reflect.TypeOf(elliptic.CurveParams{})} {
		fmt.Println(t, t.Kind(), t.NumField())
		for i := 0; i < t.NumField(); i++ {
			f := t.Field(i)
			fmt.Println(" ", f.Name, f.Type, f.Anonymous)
		}
	}
}

func main() {
	randomBytes()
	curveParams()
	keys()
	types()
}
