package main

import (
	"database/sql/driver"
	"fmt"
)

// ID is stored as its bytes, as go-ethereum's common.Hash is.
type ID [4]byte

func (id ID) Value() (driver.Value, error) {
	return id[:], nil
}

// Count is stored as an int64, and a negative one is an error.
type Count int

func (c *Count) Value() (driver.Value, error) {
	if *c < 0 {
		return nil, fmt.Errorf("negative count %d", int(*c))
	}
	return int64(*c), nil
}

// kind names the type of one of the values that a driver must handle.
func kind(v driver.Value) string {
	switch v.(type) {
	case nil:
		return "nil"
	case int64:
		return "int64"
	case float64:
		return "float64"
	case bool:
		return "bool"
	case []byte:
		return "bytes"
	case string:
		return "string"
	}
	return "other"
}

func store(v driver.Valuer) {
	value, err := v.Value()
	fmt.Println(kind(value), value, value == nil, err)
	if b, ok := value.([]byte); ok {
		fmt.Println("bytes", len(b), b)
	}
}

func main() {
	var empty driver.Value
	fmt.Println(empty == nil, empty)
	values := []driver.Value{nil, int64(-3), 2.5, true, []byte("raw"), "text"}
	for i, v := range values {
		fmt.Println(i, kind(v), v)
	}
	fmt.Printf("%T %T %T\n", values[4], values[5], values[2])
	fmt.Println(kind(ID{}))
	var a any = values[5]
	var back driver.Value = a
	fmt.Println(back == values[5], back == a, a == "text")

	store(ID{1, 2, 3, 4})
	c := Count(7)
	store(&c)
	c = -1
	store(&c)
	var v driver.Valuer = ID{}
	_, isID := v.(ID)
	fmt.Println(isID)
	var i any = &c
	if valuer, ok := i.(driver.Valuer); ok {
		store(valuer)
	}
	_, ok := any(c).(driver.Valuer)
	fmt.Println(ok)
}
