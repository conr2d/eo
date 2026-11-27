package main

import (
	"embed"
	"errors"
	"fmt"
	"io/fs"
)

//go:embed hello.txt
var greeting string

//go:embed data/sub/b.bin
var raw []byte

//go:embed hello.txt data
var files embed.FS

var none embed.FS

func show(f embed.FS, name string) {
	b, err := f.ReadFile(name)
	if err != nil {
		var pe *fs.PathError
		ok := errors.As(err, &pe)
		fmt.Printf("%q: error %v (path error %v, not exist %v)\n", name, err, ok, errors.Is(err, fs.ErrNotExist))
		return
	}
	fmt.Printf("%q: %d bytes %q (nil %v)\n", name, len(b), b, b == nil)
}

func main() {
	fmt.Printf("greeting %q\n", greeting)
	fmt.Printf("raw %d %q\n", len(raw), raw)

	names := []string{
		"hello.txt", "data/a.txt", "data/empty.txt", "data/sub/b.bin",
		"data", "data/sub", ".", "data/", "missing.txt", "data/missing",
		"/hello.txt", "./hello.txt", "data//a.txt", "data/../hello.txt", "", "hello.txt/",
		"Hello.txt", "data/sub/b.bin/x", "\xff",
	}
	for _, name := range names {
		show(files, name)
	}
	show(none, "hello.txt")
	show(none, ".")

	// A []byte variable and every ReadFile result are copies: changing one changes nothing else.
	raw[0] = 'B'
	b, _ := files.ReadFile("data/sub/b.bin")
	fmt.Printf("raw %q, file %q\n", raw[:5], b[:5])
	b[0] = 'X'
	c, _ := files.ReadFile("data/sub/b.bin")
	fmt.Printf("again %q\n", c[:5])

	// An FS is a comparable struct: a copy is equal to it, and the zero FS is not.
	copied := files
	fmt.Println(copied == files, none == files, none == embed.FS{})

	for _, p := range []string{"a", "a/b", "a/./b", "a/../b", "a//b", "/a", "a/", "..", "\xff"} {
		fmt.Printf("ValidPath(%q) = %v\n", p, fs.ValidPath(p))
	}
	pe := &fs.PathError{Op: "read", Path: "x", Err: fs.ErrInvalid}
	fmt.Println(pe.Error(), pe.Unwrap() == fs.ErrInvalid, pe.Timeout())
	fmt.Println(fs.ErrInvalid, fs.ErrPermission, fs.ErrExist, fs.ErrNotExist, fs.ErrClosed)
}
