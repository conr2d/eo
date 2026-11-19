package main

import (
	"errors"
	"fmt"
)

type NotFound struct {
	Key string
}

func (e *NotFound) Error() string {
	return "not found: " + e.Key
}

type Temporary struct {
	Code int
}

func (t Temporary) Error() string {
	return fmt.Sprintf("temporary %d", t.Code)
}

func lookup(key string) (int, error) {
	if key == "" {
		return 0, &NotFound{Key: "<empty>"}
	}
	if key == "busy" {
		return 0, Temporary{Code: 503}
	}
	return len(key), nil
}

func load(key string) (int, error) {
	n, err := lookup(key)
	if err != nil {
		return 0, fmt.Errorf("load %q: %w", key, err)
	}
	return n, nil
}

func typedNil() error {
	var p *NotFound
	return p
}

func main() {
	a := errors.New("same")
	b := errors.New("same")
	fmt.Println(a == b, a == a, a.Error() == b.Error())

	n, err := load("abc")
	fmt.Println(n, err == nil, err)

	_, err = load("")
	fmt.Println(err)
	var nf *NotFound
	found := errors.As(err, &nf)
	fmt.Println(found, nf.Key, errors.Unwrap(err) == error(nf))

	_, err = load("busy")
	var tmp Temporary
	found = errors.As(err, &tmp)
	fmt.Println(found, tmp.Code)
	fmt.Println(errors.Is(err, Temporary{Code: 503}), errors.Is(err, Temporary{Code: 500}))

	sentinel := errors.New("sentinel")
	wrapped := fmt.Errorf("layer 2: %w", fmt.Errorf("layer 1: %w", sentinel))
	fmt.Println(wrapped)
	fmt.Println(errors.Is(wrapped, sentinel), errors.Is(wrapped, errors.New("sentinel")))
	fmt.Println(errors.Unwrap(errors.Unwrap(wrapped)) == sentinel, errors.Unwrap(sentinel) == nil)

	joined := errors.Join(sentinel, nil, &NotFound{Key: "k"})
	found = errors.As(joined, &nf)
	fmt.Println(errors.Is(joined, sentinel), found, nf.Key)
	fmt.Printf("%q\n", joined.Error())
	fmt.Println(errors.Join(nil, nil) == nil)

	multi := fmt.Errorf("%w + %w", sentinel, Temporary{Code: 1})
	fmt.Println(multi, errors.Is(multi, sentinel), errors.Is(multi, Temporary{Code: 1}), errors.Unwrap(multi) == nil)

	err = typedNil()
	fmt.Println(err == nil, err)
	fmt.Printf("%T %v\n", err, err)

	var none error
	fmt.Println(none == nil, none)
	fmt.Printf("%v|%T\n", none, none)
	fmt.Println(fmt.Errorf("plain %d", 7), fmt.Errorf("bad %w", 7), fmt.Errorf("nil %w", none))
}
