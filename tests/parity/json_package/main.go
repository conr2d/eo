package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"math"
	"strconv"
	"strings"
	"sync"
)

type Inner struct {
	A int    `json:"a"`
	B string `json:"b,omitempty"`
}

type Embedded struct {
	E1 int
	E2 string `json:"e2"`
}

type Hex uint64

func (h Hex) MarshalText() ([]byte, error) {
	return []byte("0x" + strconv.FormatUint(uint64(h), 16)), nil
}

func (h *Hex) UnmarshalText(b []byte) error {
	s := string(b)
	if !strings.HasPrefix(s, "0x") {
		return errors.New("hex: missing 0x prefix")
	}
	v, err := strconv.ParseUint(s[2:], 16, 64)
	if err != nil {
		return err
	}
	*h = Hex(v)
	return nil
}

type Wrapped struct {
	V int
}

type wrappedJSON struct {
	Wrapped int `json:"wrapped"`
}

func (w Wrapped) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`{ "wrapped" : %d }`, w.V)), nil
}

func (w *Wrapped) UnmarshalJSON(b []byte) error {
	var x wrappedJSON
	if err := json.Unmarshal(b, &x); err != nil {
		return err
	}
	w.V = x.Wrapped
	return nil
}

type Config struct {
	Name    string          `json:"name"`
	Count   int             `json:"count,omitempty"`
	ID      int             `json:"id,string"`
	Ratio   float64         `json:"ratio"`
	Small   float32         `json:"small"`
	Enabled bool            `json:"enabled"`
	Tags    []string        `json:"tags"`
	Attrs   map[string]int  `json:"attrs,omitempty"`
	ByID    map[int]string  `json:"by_id"`
	Raw     json.RawMessage `json:"raw"`
	Num     json.Number     `json:"num"`
	Data    []byte          `json:"data"`
	Ptr     *Inner          `json:"ptr"`
	Inner
	*Embedded
	Hex     Hex          `json:"hex"`
	HexKeys map[Hex]bool `json:"hex_keys"`
	Wrapped Wrapped      `json:"wrapped"`
	Any     any          `json:"any"`
	Skip    int          `json:"-"`
	hidden  int
	Arr     [3]uint8 `json:"arr"`
	Zero    Inner    `json:"zero,omitzero"`
}

type Bad struct{}

func (Bad) MarshalJSON() ([]byte, error) {
	return nil, errors.New("bad")
}

type Node struct {
	Name string
	Next *Node
}

type Small struct {
	N int8
	U uint8
	F float32
	S string
}

type Tree struct {
	Value    int     `json:"value"`
	Children []*Tree `json:"children,omitempty"`
}

func newConfig() Config {
	return Config{
		Name:     "eo <&> \u2028",
		ID:       1 << 53,
		Ratio:    0.1,
		Small:    3.14,
		Enabled:  true,
		Tags:     []string{"x", "y"},
		ByID:     map[int]string{2: "two", -1: "minus one", 10: "ten"},
		Raw:      json.RawMessage(`{"k": [1, 2]}`),
		Num:      json.Number("12.50"),
		Data:     []byte("hello\x00"),
		Inner:    Inner{A: 7},
		Embedded: &Embedded{E1: 8, E2: "e"},
		Hex:      255,
		HexKeys:  map[Hex]bool{16: true, 1: false},
		Wrapped:  Wrapped{V: 3},
		Any:      []any{1, "s", nil, map[string]any{"z": 1.5}},
		Skip:     9,
		hidden:   10,
		Arr:      [3]uint8{1, 2, 3},
	}
}

func marshalling() {
	c := newConfig()
	b, err := json.Marshal(c)
	fmt.Println(string(b), err)

	b, err = json.MarshalIndent(Inner{A: 1, B: "b"}, ">", "  ")
	fmt.Println(string(b), err)

	for _, v := range []any{
		nil, true, 42, -7, uint8(200), 1e21, 1e20, 1e-7, 0.000001, float32(1e-7), math.Copysign(0, -1), 123456789.0,
		"tab\tquote\"back\\slash\x01", "bad \xff utf8", "emoji 😀",
		[]int(nil), []int{}, map[string]int(nil), &Inner{A: 2}, (*Inner)(nil),
		[]byte{}, [0]int{}, json.Number(""), Wrapped{V: 5}, &Wrapped{V: 6}, Hex(10),
	} {
		b, err := json.Marshal(v)
		fmt.Printf("%s %v\n", b, err)
	}
}

func marshalErrors() {
	for _, v := range []any{
		math.NaN(), math.Inf(-1), map[Inner]int{{A: 1}: 1}, Bad{}, []Bad{{}}, json.Number("1.2.3"),
	} {
		b, err := json.Marshal(v)
		fmt.Printf("%q %v\n", b, err)
	}

	n := &Node{Name: "loop"}
	n.Next = n
	_, err := json.Marshal(n)
	fmt.Println(err)
	var ue *json.UnsupportedValueError
	fmt.Println(errors.As(err, &ue))
}

func unmarshalling() {
	var c Config
	in := `{"NAME":"n","count":3,"id":"42","ratio":2.5,"small":1.25,"enabled":true,"tags":["a"],
		"attrs":{"k":1},"by_id":{"5":"five"},"raw":[ 1 , 2 ],"num":7,"data":"aGk=","ptr":{"a":9},
		"a":4,"b":"bee","E1":5,"e2":"ee","hex":"0x1f","hex_keys":{"0x2":true},"wrapped":{"wrapped":11},
		"any":{"list":[1,"two",false,null],"f":-1.5e3},"Skip":1,"private":2,"arr":[9],"unknown":0}`
	err := json.Unmarshal([]byte(in), &c)
	fmt.Println(err)
	fmt.Println(c.Name, c.Count, c.ID, c.Ratio, c.Small, c.Enabled, c.Tags, c.Attrs, c.ByID)
	fmt.Println(string(c.Raw), c.Num, string(c.Data), *c.Ptr, c.Inner, *c.Embedded, c.Hex, c.HexKeys, c.Wrapped)
	fmt.Println(c.Any, c.Skip, c.hidden, c.Arr)

	var m map[string]any
	err = json.Unmarshal([]byte(` {"b":[true,{"c":null}],"a":"\u00e9\ud83d\ude00\n","n":1e400} `), &m)
	fmt.Println(m, err)
	err = json.Unmarshal([]byte(`{"b":[true,{"c":null}],"a":"\u00e9\ud83d\ude00\n\/"}`), &m)
	fmt.Println(m, err)

	s := []int{1, 2, 3, 4}
	err = json.Unmarshal([]byte(`[5,6]`), &s)
	fmt.Println(s, len(s), err)
	arr := [3]int{1, 2, 3}
	err = json.Unmarshal([]byte(`[7]`), &arr)
	fmt.Println(arr, err)
	p := &Inner{A: 1}
	err = json.Unmarshal([]byte(`null`), &p)
	fmt.Println(p == nil, err)
	var x any = &Inner{}
	err = json.Unmarshal([]byte(`{"a":3}`), &x)
	fmt.Println(*x.(*Inner), err)
	var r json.RawMessage
	err = json.Unmarshal([]byte(` [1, {"a" : 2}] `), &r)
	fmt.Println(string(r), err)
	var n json.Number
	err = json.Unmarshal([]byte(`"-12e3"`), &n)
	i, ierr := n.Int64()
	f, ferr := n.Float64()
	fmt.Println(n, i, ierr, f, ferr)
}

func unmarshalErrors() {
	var c Config
	var sm Small
	var i int
	for _, in := range []string{
		`{"name":1}`, `{"N":300}`, `{"U":-1}`, `{"F":1e40}`, `{"S":[1]}`, `{"N":"1"}`, `{"by_id":{"x":"y"}}`,
		`{"hex":"zz"}`, `{"hex":12}`, `{"id":"abc"}`, `{"id":12}`, `{"ptr":{"a":"s"}}`,
		`{"a":1,}`, `[1,2`, `{"a" 1}`, `tru`, `01`, `"\x"`, ``, `  `, `{}x`, `1 2`,
	} {
		var err error
		switch {
		case strings.Contains(in, `"N"`), strings.Contains(in, `"U"`), strings.Contains(in, `"F"`), strings.Contains(in, `"S"`):
			err = json.Unmarshal([]byte(in), &sm)
		case strings.HasPrefix(in, "{"):
			err = json.Unmarshal([]byte(in), &c)
		default:
			err = json.Unmarshal([]byte(in), &i)
		}
		fmt.Printf("%q: %v\n", in, err)
		if se, ok := err.(*json.SyntaxError); ok {
			fmt.Println("  offset", se.Offset)
		}
		if te, ok := err.(*json.UnmarshalTypeError); ok {
			fmt.Println("  type", te.Value, te.Type, te.Offset, te.Struct, te.Field)
		}
	}
	var none, value any = nil, i
	var np *int
	for _, target := range []any{none, value, np} {
		fmt.Println(json.Unmarshal([]byte(`1`), target))
	}
}

func helpers() {
	fmt.Println(json.Valid([]byte(`{"a":[1,2,{"b":null}]}`)), json.Valid([]byte(`{"a":}`)), json.Valid(nil))

	var buf bytes.Buffer
	err := json.Compact(&buf, []byte(" { \"a\" : [ 1 , 2 ] ,\n \"b\" : \"<x>\" } "))
	fmt.Println(buf.String(), err)
	buf.Reset()
	err = json.Indent(&buf, []byte(`{"a":[1,2,{}],"b":[],"c":{"d":"e"}}`), "", "\t")
	fmt.Println(buf.String(), err)
	buf.Reset()
	err = json.Indent(&buf, []byte(`{"a":[1,2`), "", "\t")
	fmt.Printf("%q %v\n", buf.String(), err)
	buf.Reset()
	json.HTMLEscape(&buf, []byte(`{"h":"<b>&amp;</b>`+"\u2029"+`"}`))
	fmt.Println(buf.String())
}

func streams() {
	var buf bytes.Buffer
	enc := json.NewEncoder(&buf)
	fmt.Println(enc.Encode(map[string]any{"html": "<&>", "n": 1}))
	enc.SetEscapeHTML(false)
	enc.SetIndent("", " ")
	fmt.Println(enc.Encode(map[string]any{"html": "<&>", "list": []int{1, 2}}))
	fmt.Println(enc.Encode(math.NaN()))
	fmt.Print(buf.String())

	dec := json.NewDecoder(strings.NewReader(` {"a":1} [2,3] "four" 5.5 null {"b":`))
	for {
		var v any
		err := dec.Decode(&v)
		fmt.Println(v, err, dec.InputOffset())
		if err != nil {
			break
		}
	}

	dec = json.NewDecoder(strings.NewReader(`{"x":1.0,"y":[10,20]}`))
	dec.UseNumber()
	var v map[string]any
	err := dec.Decode(&v)
	fmt.Println(err, v)
	fmt.Printf("%T\n", v["x"])

	dec = json.NewDecoder(strings.NewReader(`{"a":1,"zzz":2}`))
	dec.DisallowUnknownFields()
	var in Inner
	err = dec.Decode(&in)
	fmt.Println(err, in)

	dec = json.NewDecoder(strings.NewReader(`{"k":[1,"s",{"n":null}],"t":true} rest`))
	for {
		tok, err := dec.Token()
		if err != nil {
			fmt.Println("token error:", err)
			break
		}
		fmt.Printf("%T %v more=%v\n", tok, tok, dec.More())
	}

	dec = json.NewDecoder(strings.NewReader(`[{"a":1},{"a":2},{"a":3}]`))
	tok, err := dec.Token()
	fmt.Println(tok, err)
	for dec.More() {
		var e Inner
		err := dec.Decode(&e)
		fmt.Println(e, err)
	}
	tok, err = dec.Token()
	fmt.Println(tok, err)
	tok, err = dec.Token()
	fmt.Println(tok, err)

	dec = json.NewDecoder(strings.NewReader(`{"a":1} tail`))
	var first map[string]int
	err = dec.Decode(&first)
	fmt.Println(err, first)
	rest := new(bytes.Buffer)
	rest.ReadFrom(dec.Buffered())
	fmt.Printf("%q\n", rest.String())
}

func concurrent() {
	var wg sync.WaitGroup
	results := make([]string, 8)
	for g := 0; g < 8; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			b, err := json.Marshal(&Tree{Value: g, Children: []*Tree{{Value: g * 10}}})
			if err != nil {
				panic(err)
			}
			results[g] = string(b)
		}()
	}
	wg.Wait()
	fmt.Println(results)
}

func main() {
	marshalling()
	marshalErrors()
	unmarshalling()
	unmarshalErrors()
	helpers()
	streams()
	concurrent()
}
