package main

import (
	"errors"
	"fmt"
	"reflect"
	"slices"
)

type Point struct {
	X, Y int
}

type Base struct {
	ID int
}

func (b Base) Describe() string {
	return fmt.Sprintf("base %d", b.ID)
}

type Tagged struct {
	Name   string `json:"name" xml:"n"`
	Count  int    `json:"count,omitempty"`
	hidden bool
	Base
	Next  *Tagged
	Tags  []string
	Attrs map[string]int
	Err   error
	Any   any
	Raw   [4]byte
	F     float64
}

type Celsius float64

func (c Celsius) String() string {
	return fmt.Sprintf("%.1fC", float64(c))
}

type Bytes []byte

type Outer struct {
	*Point
}

type Stringer interface {
	String() string
}

func kinds() {
	for _, k := range []reflect.Kind{reflect.Invalid, reflect.Bool, reflect.Int, reflect.Uint8, reflect.Float64,
		reflect.Array, reflect.Interface, reflect.Map, reflect.Pointer, reflect.Slice, reflect.String, reflect.Struct,
		reflect.UnsafePointer, reflect.Kind(99)} {
		fmt.Println(int(k), k.String(), k)
	}
	fmt.Println(reflect.Ptr == reflect.Pointer)
}

func types() {
	values := []any{true, 1, int8(2), int16(3), int32(4), uint(6), uint8(7), uint16(8), uint32(9), float32(1.5),
		2.5, "s", []int{1}, [2]string{}, map[string]bool{}, &Point{}, Point{}, Celsius(1), Bytes{1}, struct{}{}}
	for _, v := range values {
		t := reflect.TypeOf(v)
		fmt.Printf("%s|%s|%s|%v\n", t.String(), t.Name(), t.Kind(), t.Comparable())
	}
	fmt.Println(reflect.TypeOf(nil) == nil)
	fmt.Println(reflect.TypeOf([]int{}).Elem(), reflect.TypeOf(map[string]bool{}).Key(),
		reflect.TypeOf(map[string]bool{}).Elem(), reflect.TypeOf([3]int{}).Len(), reflect.TypeOf(&Point{}).Elem().Name())
	fmt.Println(reflect.TypeOf(int8(0)).Bits(), reflect.TypeOf(float32(0)).Bits(), reflect.TypeOf(0).Bits())

	errType := reflect.TypeFor[error]()
	anyType := reflect.TypeFor[any]()
	stringer := reflect.TypeFor[Stringer]()
	fmt.Printf("%s|%s|%s|%s|%q|%s\n", errType, errType.Kind(), stringer, stringer.Name(), anyType.Name(), anyType)
	fmt.Println(reflect.TypeOf(Celsius(0)).Implements(stringer), reflect.TypeOf(Point{}).Implements(stringer),
		errType.Implements(anyType), stringer.Implements(errType))
	fmt.Println(reflect.TypeOf(errors.New("x")).Implements(errType), reflect.TypeOf(errors.New("x")))
	fmt.Println(reflect.PointerTo(reflect.TypeOf(Celsius(0))).Implements(stringer),
		reflect.PointerTo(reflect.TypeOf(Point{})) == reflect.TypeOf(&Point{}), reflect.PtrTo(reflect.TypeOf(0)))
	fmt.Println(reflect.TypeOf(Point{}).NumMethod(), reflect.TypeOf(Base{}).NumMethod(),
		reflect.TypeOf(Tagged{}).NumMethod(), stringer.NumMethod(), anyType.NumMethod())
	fmt.Println(reflect.TypeOf(Bytes{}).AssignableTo(reflect.TypeOf([]byte{})),
		reflect.TypeOf([]byte{}).AssignableTo(reflect.TypeOf(Bytes{})),
		reflect.TypeOf(Celsius(0)).AssignableTo(reflect.TypeOf(0.0)), reflect.TypeOf(Point{}).AssignableTo(anyType))
	fmt.Println(reflect.TypeOf(Tagged{}).Comparable(), reflect.TypeOf([2]Point{}).Comparable(), errType.Comparable())
	pp := reflect.PointerTo(reflect.TypeOf(&Point{}))
	fmt.Println(pp, pp.Elem() == reflect.TypeOf(&Point{}), pp.Elem().Elem() == reflect.TypeOf(Point{}),
		reflect.PointerTo(reflect.TypeOf(Celsius(0))) == reflect.TypeOf(new(Celsius)))
}

func structFields() {
	t := reflect.TypeOf(Tagged{})
	fmt.Println(t.NumField())
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		fmt.Printf("%d %s %q %s exported=%v anonymous=%v index=%v json=%q\n", i, f.Name, f.PkgPath, f.Type,
			f.IsExported(), f.Anonymous, f.Index, f.Tag.Get("json"))
	}
	v, ok := t.Field(0).Tag.Lookup("xml")
	fmt.Println(v, ok)
	v, ok = t.Field(0).Tag.Lookup("yaml")
	fmt.Printf("%q %v\n", v, ok)
	fmt.Println(reflect.StructTag(`a:"1"  b:"two \"quoted\""`).Get("b"))
	v, ok = reflect.StructTag(`bad`).Lookup("bad")
	fmt.Printf("%q %v\n", v, ok)
	fmt.Println(reflect.StructTag(`a:"\x41\u00e9"`).Get("a"))
}

func values() {
	p := &Point{1, 2}
	v := reflect.ValueOf(p)
	fmt.Println(v.Kind(), v.Type(), v.CanAddr(), v.CanSet())
	e := v.Elem()
	fmt.Println(e.Kind(), e.Type(), e.CanAddr(), e.CanSet(), e.NumField())
	e.Field(0).SetInt(10)
	e.FieldByIndex([]int{1}).SetInt(20)
	fmt.Println(*p, e.Field(0).Int(), e.Field(1).Interface())
	w := reflect.ValueOf(Point{3, 4})
	fmt.Println(w.CanAddr(), w.Field(0).CanSet(), w.Field(1).Int())
	fmt.Println(reflect.ValueOf(nil).IsValid(), reflect.ValueOf(0).IsValid())
	fmt.Println(reflect.ValueOf("hi").String(), reflect.ValueOf(3).String(), reflect.Value{}.String())
	fmt.Println(reflect.ValueOf(true).Bool(), reflect.ValueOf(uint8(200)).Uint(), reflect.ValueOf(float32(1.25)).Float(),
		reflect.ValueOf(int16(-3)).Int())
	fmt.Println(reflect.ValueOf(Celsius(2.5)).Float(), reflect.ValueOf(Celsius(2.5)).Kind())
	fmt.Println(reflect.ValueOf(p).Interface().(*Point) == p)
	fmt.Println(reflect.ValueOf(Point{5, 6}), reflect.ValueOf(Celsius(1)))
	fmt.Println(reflect.Value{})

	a := e.Field(0).Addr()
	fmt.Println(a.Type(), *a.Interface().(*int))
	*a.Interface().(*int) = 99
	fmt.Println(p.X)
	fmt.Println(reflect.Indirect(v).Field(0), reflect.Indirect(reflect.ValueOf(7)))

	n := reflect.New(reflect.TypeOf(Point{}))
	n.Elem().Field(1).SetInt(7)
	fmt.Println(n.Type(), n.Elem().Interface(), n.Interface().(*Point).Y)
	z := reflect.Zero(reflect.TypeOf(Point{}))
	fmt.Println(z, z.IsZero(), z.CanSet())

	negZero := 0.0
	negZero = -negZero
	fmt.Println(reflect.ValueOf(0.0).IsZero(), reflect.ValueOf(negZero).IsZero(), reflect.ValueOf([2]int{0, 1}).IsZero(),
		reflect.ValueOf(Point{}).IsZero(), reflect.ValueOf("").IsZero(), reflect.ValueOf((*int)(nil)).IsZero(),
		reflect.ValueOf(Tagged{}).IsZero(), reflect.ValueOf(Tagged{Raw: [4]byte{0, 0, 1, 0}}).IsZero())

	var np *Point
	var ns []int
	var nm map[string]int
	var ne error
	fmt.Println(reflect.ValueOf(np).IsNil(), reflect.ValueOf(ns).IsNil(), reflect.ValueOf(nm).IsNil(),
		reflect.ValueOf(&ne).Elem().IsNil(), reflect.ValueOf(&ne).Elem().Kind(), reflect.ValueOf(&ne).Elem().Elem().IsValid())

	fmt.Println(reflect.ValueOf(int8(0)).OverflowInt(200), reflect.ValueOf(int8(0)).OverflowInt(-128),
		reflect.ValueOf(uint16(0)).OverflowUint(70000), reflect.ValueOf(float32(0)).OverflowFloat(1e39),
		reflect.ValueOf(0.0).OverflowFloat(1e300), reflect.ValueOf(0).OverflowInt(-1<<62))

	x, ok := reflect.TypeAssert[int](reflect.ValueOf(5))
	fmt.Println(x, ok)
	s, ok := reflect.TypeAssert[string](reflect.ValueOf(5))
	fmt.Printf("%q %v\n", s, ok)
	st, ok := reflect.TypeAssert[fmt.Stringer](reflect.ValueOf(Celsius(3)))
	fmt.Println(st, ok)
	_, ok = reflect.TypeAssert[fmt.Stringer](reflect.ValueOf(Point{}))
	fmt.Println(ok)
}

func setters() {
	var t Tagged
	v := reflect.ValueOf(&t).Elem()
	v.FieldByIndex([]int{0}).SetString("n")
	v.Field(1).SetInt(3)
	v.Field(3).Field(0).SetInt(42)
	v.Field(5).Set(reflect.ValueOf([]string{"a", "b"}))
	v.Field(6).Set(reflect.MakeMap(v.Field(6).Type()))
	v.Field(6).SetMapIndex(reflect.ValueOf("k"), reflect.ValueOf(1))
	v.Field(7).Set(reflect.ValueOf(errors.New("boom")))
	v.Field(8).Set(reflect.ValueOf(Point{1, 1}))
	v.Field(10).SetFloat(0.5)
	v.Field(4).Set(reflect.New(v.Field(4).Type().Elem()))
	v.Field(4).Elem().Field(0).SetString("child")
	fmt.Println(t.Name, t.Count, t.ID, t.Tags, t.Attrs, t.Err, t.Any, t.F, t.Next.Name, t.Describe())
	fmt.Println(v.Field(7).Kind(), v.Field(7).Elem().Type(), v.Field(8).Elem().Type(), v.Field(8).Elem().Field(0),
		v.Field(8).NumMethod(), v.Field(7).NumMethod())
	v.Field(8).Set(reflect.Zero(v.Field(8).Type()))
	fmt.Println(t.Any == nil)
	v.Field(8).Set(reflect.ValueOf(&t.Count))
	*t.Any.(*int) = 4
	fmt.Println(t.Count)
	fmt.Println(v.Field(2).CanSet(), v.Field(2).CanInterface(), v.Field(2).Bool(), v.Field(2).CanAddr())
	v.Field(0).SetZero()
	fmt.Printf("%q\n", t.Name)

	r := v.Field(9).Bytes()
	r[1] = 9
	fmt.Println(t.Raw)
	fmt.Println(reflect.Copy(v.Field(9), reflect.ValueOf("abcdef")), t.Raw)

	var b Bytes
	bv := reflect.ValueOf(&b).Elem()
	bv.Set(reflect.ValueOf([]byte("xy")))
	fmt.Println(b, reflect.ValueOf(b).Bytes(), bv.Type(), bv.Kind())
	bv.SetBytes([]byte("z"))
	fmt.Println(string(b))
	var raw []byte
	reflect.ValueOf(&raw).Elem().Set(reflect.ValueOf(Bytes("w")))
	fmt.Println(string(raw))
	var c Celsius
	reflect.ValueOf(&c).Elem().SetFloat(36.6)
	fmt.Println(c)
	var u uint16
	reflect.ValueOf(&u).Elem().SetUint(70000)
	fmt.Println(u)
}

func slicesAndArrays() {
	s := reflect.MakeSlice(reflect.TypeOf([]int{}), 2, 5)
	fmt.Println(s.Len(), s.Cap(), s.Interface(), s.IsNil())
	s.Index(1).SetInt(7)
	fmt.Println(s.Interface(), s.Index(1).CanSet())
	sp := reflect.New(s.Type())
	sp.Elem().Set(s)
	e := sp.Elem()
	e.SetLen(4)
	fmt.Println(e.Len(), e.Interface())
	e.Grow(10)
	fmt.Println(e.Len(), e.Cap() >= 14, e.Interface())
	e.SetLen(e.Len() + 1)
	e.Index(4).SetInt(5)
	fmt.Println(e.Interface(), s.Interface())

	arr := [3]string{"a", "b", "c"}
	av := reflect.ValueOf(&arr).Elem()
	av.Index(0).SetString("A")
	fmt.Println(arr, av.Len(), av.Cap(), reflect.ValueOf(arr).Index(2), reflect.ValueOf(arr).Index(2).CanSet())
	fmt.Println(reflect.ValueOf("héllo").Len(), reflect.ValueOf("hey").Index(1).Uint(), reflect.ValueOf(&arr).Len())

	ints := []int{1, 2, 3, 4, 5}
	n := reflect.Copy(reflect.ValueOf(ints[1:]), reflect.ValueOf(ints))
	fmt.Println(n, ints)
	ints = []int{1, 2, 3, 4, 5}
	n = reflect.Copy(reflect.ValueOf(ints), reflect.ValueOf(ints[2:]))
	fmt.Println(n, ints)
	dst := make([]byte, 3)
	fmt.Println(reflect.Copy(reflect.ValueOf(dst), reflect.ValueOf("xyzw")), string(dst))
	fmt.Println(reflect.MakeSlice(reflect.TypeOf(Bytes{}), 0, 0).Interface(), reflect.MakeSlice(reflect.TypeOf(Bytes{}), 0, 0).IsNil())
}

func maps() {
	m := map[string]int{"a": 1, "b": 2}
	mv := reflect.ValueOf(m)
	fmt.Println(mv.Len(), mv.MapIndex(reflect.ValueOf("a")), mv.MapIndex(reflect.ValueOf("z")).IsValid())
	var names []string
	for _, k := range mv.MapKeys() {
		names = append(names, k.String())
	}
	slices.SortFunc(names, func(a, b string) int {
		if a < b {
			return -1
		}
		if a > b {
			return 1
		}
		return 0
	})
	fmt.Println(names)
	mv.SetMapIndex(reflect.ValueOf("c"), reflect.ValueOf(3))
	mv.SetMapIndex(reflect.ValueOf("a"), reflect.Value{})
	fmt.Println(m)
	it := mv.MapRange()
	sum, count := 0, 0
	for it.Next() {
		sum += int(it.Value().Int())
		count += len(it.Key().String())
	}
	fmt.Println(count, sum)
	mm := reflect.MakeMapWithSize(reflect.TypeOf(map[int]string{}), 4)
	mm.SetMapIndex(reflect.ValueOf(1), reflect.ValueOf("one"))
	fmt.Println(mm.Interface(), mm.Len(), len(reflect.ValueOf(map[int]int{}).MapKeys()))
	var nilMap map[string]int
	fmt.Println(reflect.ValueOf(nilMap).MapIndex(reflect.ValueOf("x")).IsValid(), reflect.ValueOf(nilMap).Len())
	found := reflect.ValueOf(map[bool]int{false: 7}).MapIndex(reflect.ValueOf(Tagged{}).Field(2))
	fmt.Println(found.Int(), found.CanInterface())
}

func attempt(name string, f func()) {
	defer func() { fmt.Println(name, "recovered:", recover()) }()
	f()
}

func panics() {
	attempt("Int of string", func() { reflect.ValueOf("s").Int() })
	attempt("Type of zero", func() { reflect.Value{}.Type() })
	attempt("SetInt unaddressable", func() { reflect.ValueOf(1).SetInt(2) })
	attempt("SetBool unexported", func() { reflect.ValueOf(&Tagged{}).Elem().Field(2).SetBool(true) })
	attempt("Interface unexported", func() { reflect.ValueOf(Tagged{}).Field(2).Interface() })
	attempt("Set not assignable", func() {
		var x int
		reflect.ValueOf(&x).Elem().Set(reflect.ValueOf("s"))
	})
	attempt("Set zero Value", func() {
		var x int
		reflect.ValueOf(&x).Elem().Set(reflect.Value{})
	})
	attempt("Elem of int type", func() { reflect.TypeOf(1).Elem() })
	attempt("Elem of int", func() { reflect.ValueOf(1).Elem() })
	attempt("Field out of range", func() { reflect.ValueOf(Point{}).Field(5) })
	attempt("Type.Field out of range", func() { reflect.TypeOf(Point{}).Field(5) })
	attempt("Index out of range", func() { reflect.ValueOf([]int{1}).Index(3) })
	attempt("SetLen out of range", func() {
		s := []int{1}
		reflect.ValueOf(&s).Elem().SetLen(5)
	})
	attempt("Bytes of ints", func() { reflect.ValueOf([]int{1}).Bytes() })
	attempt("Bytes of unaddressable array", func() { reflect.ValueOf([2]byte{}).Bytes() })
	attempt("New nil", func() { reflect.New(nil) })
	attempt("MakeSlice of map", func() { reflect.MakeSlice(reflect.TypeOf(map[int]int{}), 0, 0) })
	attempt("MakeSlice len > cap", func() { reflect.MakeSlice(reflect.TypeOf([]int{}), 2, 1) })
	attempt("Implements non-interface", func() { reflect.TypeOf(1).Implements(reflect.TypeOf(1)) })
	attempt("NumField of int", func() { reflect.TypeOf(1).NumField() })
	attempt("Key of slice", func() { reflect.TypeOf([]int{}).Key() })
	attempt("Len of slice type", func() { reflect.TypeOf([]int{}).Len() })
	attempt("Bits of string", func() { reflect.TypeOf("").Bits() })
	attempt("MapIter exhausted", func() {
		it := reflect.ValueOf(map[int]int{}).MapRange()
		it.Next()
		it.Next()
	})
	attempt("MapIter Key before Next", func() { reflect.ValueOf(map[int]int{}).MapRange().Key() })
	attempt("SetMapIndex wrong key", func() {
		reflect.ValueOf(map[int]int{}).SetMapIndex(reflect.ValueOf("k"), reflect.ValueOf(1))
	})
	attempt("SetMapIndex nil map", func() {
		var m map[int]int
		reflect.ValueOf(m).SetMapIndex(reflect.ValueOf(1), reflect.ValueOf(1))
	})
	attempt("Copy mismatch", func() { reflect.Copy(reflect.ValueOf([]int{1}), reflect.ValueOf([]string{"a"})) })
	attempt("Copy into int", func() { reflect.Copy(reflect.ValueOf(1), reflect.ValueOf([]int{1})) })
	attempt("IsNil of int", func() { reflect.ValueOf(1).IsNil() })
	attempt("IsZero of zero", func() { reflect.Value{}.IsZero() })
	attempt("Addr unaddressable", func() { reflect.ValueOf(1).Addr() })
	attempt("Len of int", func() { reflect.ValueOf(1).Len() })
	attempt("Grow negative", func() {
		s := []int{}
		reflect.ValueOf(&s).Elem().Grow(-1)
	})
	attempt("Float of int", func() { reflect.ValueOf(1).Float() })
	attempt("SetString of int", func() {
		var x int
		reflect.ValueOf(&x).Elem().SetString("s")
	})
	attempt("TypeAssert zero", func() { reflect.TypeAssert[int](reflect.Value{}) })
	attempt("FieldByIndex nil embedded", func() { reflect.ValueOf(Outer{}).FieldByIndex([]int{0, 1}) })
}

func main() {
	kinds()
	types()
	structFields()
	values()
	setters()
	slicesAndArrays()
	maps()
	panics()
}
