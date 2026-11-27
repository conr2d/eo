package main

import (
	"encoding/json"
	"fmt"
	"math/big"
	"reflect"
)

type Amount big.Int

type Record struct {
	Value big.Int
	Ptr   *big.Int
	Name  string
}

func equal(a, b big.Int) {
	defer func() {
		fmt.Println("recovered:", recover())
	}()
	fmt.Println(reflect.ValueOf(a).Equal(reflect.ValueOf(b)))
}

func main() {
	t := reflect.TypeFor[big.Int]()
	fmt.Println(t.Kind(), t.Kind() == reflect.Struct, reflect.TypeFor[Amount]().Kind(), reflect.TypeFor[*big.Int]().Elem().Kind())

	var zero big.Int
	five := big.NewInt(5)
	fmt.Println(reflect.ValueOf(zero).IsZero(), reflect.ValueOf(*five).IsZero(), reflect.ValueOf(five).Elem().IsZero())
	r := Record{Ptr: big.NewInt(7), Name: "r"}
	fmt.Println(reflect.ValueOf(r).Field(0).IsZero(), reflect.ValueOf(r).Field(0).Kind())

	out, err := json.Marshal(r)
	fmt.Println(string(out), err)
	out, err = json.Marshal(map[string]big.Int{"a": *five})
	fmt.Println(string(out), err)
	var back Record
	err = json.Unmarshal([]byte(`{"Value":"0x1f","Ptr":12,"Name":"b"}`), &back)
	fmt.Println(err, back.Value.String(), back.Ptr, back.Name)
	err = json.Unmarshal([]byte(`{"Ptr":null}`), &back)
	fmt.Println(err, back.Ptr)
	err = json.Unmarshal([]byte(`{"Value":{"x":1}}`), &back)
	fmt.Println(err)
	text, err := five.MarshalText()
	fmt.Println(string(text), err)
	var nilInt *big.Int
	text, err = nilInt.MarshalText()
	fmt.Println(string(text), err)
	text, err = nilInt.MarshalJSON()
	fmt.Println(string(text), err)
	text, err = big.NewInt(-12).AppendText([]byte("n="))
	fmt.Println(string(text), err)
	z := new(big.Int)
	fmt.Println(z.UnmarshalText([]byte("0b101")), z, z.UnmarshalText([]byte("1_000")), z, z.UnmarshalText([]byte("12x")))
	fmt.Println(z.UnmarshalJSON([]byte("null")), z.UnmarshalJSON([]byte("-77")), z)
	equal(*big.NewInt(1), *big.NewInt(1))
	equal(*big.NewInt(-1), *big.NewInt(1))
	equal(*big.NewInt(-3), *big.NewInt(-1))
}
