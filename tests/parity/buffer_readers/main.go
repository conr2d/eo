package main

import (
	"bytes"
	"fmt"
	"io"
	"strings"
)

// A linear congruential generator, so that both programs draw the same values.
var state uint64 = 88172645463325252

func next() uint64 {
	state = state*6364136223846793005 + 1442695040888963407
	return state
}

func pick(n int) int {
	return int(next() >> 33 % uint64(n))
}

var pieces = []string{"a", "bc", "\n", "é", "☺", "\xff", "\xe2\x98", "xyz\n", "", "0123456789"}

var runes = []rune{'q', 0xE9, 0x263A, 0x1F600, -1, 0xD800}

func attempt(name string, f func()) {
	defer func() { fmt.Println(name, "recovered:", recover()) }()
	f()
}

func buffers() {
	var b bytes.Buffer
	fmt.Println(b.Len(), b.String() == "", b.Bytes() == nil)
	n, err := b.WriteString("hello, ")
	fmt.Println(n, err)
	n, err = b.Write([]byte("world"))
	fmt.Println(n, err)
	err = b.WriteByte('!')
	fmt.Println(err, b.Len())
	n, err = b.WriteRune(0x263A)
	fmt.Println(n, err, b.String())
	p := make([]byte, 5)
	n, err = b.Read(p)
	fmt.Println(n, err, string(p[:n]), b.Len())
	fmt.Println(b.UnreadByte(), b.Len())
	fmt.Println(b.UnreadByte())
	c, err := b.ReadByte()
	fmt.Println(c, err)
	nx := b.Next(3)
	fmt.Println(string(nx), b.Len())
	peek, err := b.Peek(2)
	fmt.Println(string(peek), err)
	peek, err = b.Peek(100)
	fmt.Println(len(peek), err)
	line, err := b.ReadString('d')
	fmt.Println(line, err)
	r, size, err := b.ReadRune()
	fmt.Println(r, size, err)
	r, size, err = b.ReadRune()
	fmt.Println(r, size, err)
	fmt.Println(b.UnreadRune(), b.Len())
	fmt.Println(b.UnreadRune())
	r, size, err = b.ReadRune()
	fmt.Println(r, size, err, b.Len())
	r, size, err = b.ReadRune()
	fmt.Println(r, size, err)
	c, err = b.ReadByte()
	fmt.Println(c, err)
	n, err = b.Read(p)
	fmt.Println(n, err)
	n, err = b.Read(nil)
	fmt.Println(n, err)
	fmt.Println(b.UnreadByte())

	b.WriteString("one\ntwo\nthree")
	for {
		bs, err := b.ReadBytes('\n')
		fmt.Printf("%q %v\n", bs, err)
		if err != nil {
			break
		}
	}
	b.WriteString("truncate me")
	b.Truncate(8)
	fmt.Println(b.String())
	b.Truncate(0)
	fmt.Println(b.Len(), b.String() == "")
	attempt("truncate", func() { b.Truncate(1) })
	attempt("grow", func() { b.Grow(-1) })

	nb := bytes.NewBufferString("from string")
	fmt.Println(nb.String(), nb.Len())
	nb = bytes.NewBuffer([]byte("from bytes"))
	var sink bytes.Buffer
	n64, err := nb.WriteTo(&sink)
	fmt.Println(n64, err, sink.String(), nb.Len())
	n64, err = sink.ReadFrom(strings.NewReader(" and more"))
	fmt.Println(n64, err, sink.String())
	var nilBuf *bytes.Buffer
	fmt.Println(nilBuf.String())
	fmt.Println(&sink)

	// Bytes aliases the buffer until the next change.
	fixed := bytes.NewBuffer(make([]byte, 0, 16))
	fixed.WriteString("abcdef")
	view := fixed.Bytes()
	view[0] = 'A'
	fmt.Println(fixed.String(), fixed.Cap(), fixed.Available())
	fixed.WriteString("gh")
	fmt.Println(string(view), fixed.String(), len(fixed.AvailableBuffer()))
	fixed.Grow(4)
	fmt.Println(fixed.Cap(), fixed.Len())
	fixed.Reset()
	fmt.Println(fixed.Len(), fixed.Cap())
}

func bytesReader() {
	r := bytes.NewReader([]byte("héllo, wörld"))
	fmt.Println(r.Len(), r.Size())
	p := make([]byte, 4)
	n, err := r.Read(p)
	fmt.Printf("%d %v %q %d\n", n, err, p[:n], r.Len())
	ch, size, err := r.ReadRune()
	fmt.Println(ch, size, err)
	fmt.Println(r.UnreadRune(), r.Len())
	fmt.Println(r.UnreadRune())
	c, err := r.ReadByte()
	fmt.Println(c, err)
	fmt.Println(r.UnreadByte(), r.Len())
	pos, err := r.Seek(-3, io.SeekEnd)
	fmt.Println(pos, err)
	ch, size, err = r.ReadRune()
	fmt.Println(ch, size, err)
	pos, err = r.Seek(2, io.SeekCurrent)
	fmt.Println(pos, err, r.Len())
	n, err = r.Read(p)
	fmt.Println(n, err)
	pos, err = r.Seek(-1, io.SeekStart)
	fmt.Println(pos, err)
	pos, err = r.Seek(0, 9)
	fmt.Println(pos, err)
	n, err = r.ReadAt(p, 9)
	fmt.Printf("%d %v %q\n", n, err, p[:n])
	n, err = r.ReadAt(p, 2)
	fmt.Printf("%d %v %q\n", n, err, p[:n])
	n, err = r.ReadAt(p, 100)
	fmt.Println(n, err)
	n, err = r.ReadAt(p, -1)
	fmt.Println(n, err)
	r.Seek(7, io.SeekStart)
	var sink bytes.Buffer
	n64, err := r.WriteTo(&sink)
	fmt.Println(n64, err, sink.String(), r.Len())
	n64, err = r.WriteTo(&sink)
	fmt.Println(n64, err)
	r.Reset([]byte("reset"))
	b, err := io.ReadAll(r)
	fmt.Println(string(b), err)
	empty := bytes.NewReader(nil)
	fmt.Println(empty.UnreadByte())
	fmt.Println(empty.UnreadRune())
	_, _, err = empty.ReadRune()
	fmt.Println(err)
	r = bytes.NewReader([]byte("\xff\xe2\x98z"))
	for {
		ch, size, err := r.ReadRune()
		if err != nil {
			break
		}
		fmt.Println(ch, size)
	}
	r = bytes.NewReader([]byte("ab"))
	r.Seek(10, io.SeekStart)
	fmt.Println(r.Len())
	n, err = r.Read(p)
	fmt.Println(n, err)
}

func stringsReader() {
	r := strings.NewReader("héllo, wörld")
	fmt.Println(r.Len(), r.Size())
	p := make([]byte, 4)
	n, err := r.Read(p)
	fmt.Printf("%d %v %q %d\n", n, err, p[:n], r.Len())
	ch, size, err := r.ReadRune()
	fmt.Println(ch, size, err)
	fmt.Println(r.UnreadRune(), r.Len())
	fmt.Println(r.UnreadRune())
	c, err := r.ReadByte()
	fmt.Println(c, err)
	fmt.Println(r.UnreadByte(), r.Len())
	pos, err := r.Seek(-3, io.SeekEnd)
	fmt.Println(pos, err)
	n, err = r.ReadAt(p, 9)
	fmt.Printf("%d %v %q\n", n, err, p[:n])
	n, err = r.ReadAt(p, -1)
	fmt.Println(n, err)
	pos, err = r.Seek(-1, io.SeekCurrent)
	fmt.Println(pos, err)
	pos, err = r.Seek(0, 3)
	fmt.Println(pos, err)
	var sink bytes.Buffer
	n64, err := r.WriteTo(&sink)
	fmt.Println(n64, err, sink.String())
	r.Reset("again")
	pw := &plainWriter{}
	n64, err = r.WriteTo(pw)
	fmt.Println(n64, err, pw.got)
	empty := strings.NewReader("")
	fmt.Println(empty.UnreadByte())
	fmt.Println(empty.UnreadRune())
}

// A Writer that only has Write.
type plainWriter struct {
	got string
}

func (w *plainWriter) Write(p []byte) (int, error) {
	w.got += string(p)
	return len(p), nil
}

// A random sequence of Buffer operations, each of which prints its results.
func randomBuffer() {
	var b bytes.Buffer
	for i := 0; i < 400; i++ {
		var out string
		switch pick(14) {
		case 0:
			n, err := b.WriteString(pieces[pick(len(pieces))])
			out = fmt.Sprint("WriteString ", n, err)
		case 1:
			n, err := b.Write([]byte(pieces[pick(len(pieces))]))
			out = fmt.Sprint("Write ", n, err)
		case 2:
			err := b.WriteByte(byte(pick(256)))
			out = fmt.Sprint("WriteByte ", err)
		case 3:
			n, err := b.WriteRune(runes[pick(len(runes))])
			out = fmt.Sprint("WriteRune ", n, err)
		case 4:
			p := make([]byte, pick(6))
			n, err := b.Read(p)
			out = fmt.Sprintf("Read %d %v %q", n, err, p[:n])
		case 5:
			c, err := b.ReadByte()
			out = fmt.Sprint("ReadByte ", c, err)
		case 6:
			r, size, err := b.ReadRune()
			out = fmt.Sprint("ReadRune ", r, size, err)
		case 7:
			out = fmt.Sprint("UnreadByte ", b.UnreadByte())
		case 8:
			out = fmt.Sprint("UnreadRune ", b.UnreadRune())
		case 9:
			out = fmt.Sprintf("Next %q", b.Next(pick(5)))
		case 10:
			line, err := b.ReadString('\n')
			out = fmt.Sprintf("ReadString %q %v", line, err)
		case 11:
			if b.Len() > 0 {
				b.Truncate(pick(b.Len()))
			}
			out = "Truncate"
		case 12:
			b.Grow(pick(100))
			out = "Grow"
		case 13:
			peek, err := b.Peek(pick(4))
			out = fmt.Sprintf("Peek %q %v", peek, err)
		}
		fmt.Printf("%s | %d %q\n", out, b.Len(), b.String())
	}
}

// A random sequence of Reader operations on the same input, through bytes.Reader and
// strings.Reader, each of which prints its results.
func randomReaders() {
	input := ""
	for i := 0; i < 12; i++ {
		input += pieces[pick(len(pieces))]
	}
	br := bytes.NewReader([]byte(input))
	sr := strings.NewReader(input)
	for i := 0; i < 300; i++ {
		op := pick(8)
		var a, c string
		switch op {
		case 0:
			k := pick(5)
			p := make([]byte, k)
			n, err := br.Read(p)
			a = fmt.Sprintf("Read %d %v %q", n, err, p[:n])
			q := make([]byte, k)
			n, err = sr.Read(q)
			c = fmt.Sprintf("Read %d %v %q", n, err, q[:n])
		case 1:
			k := pick(5)
			off := int64(pick(len(input)+3)) - 1
			p := make([]byte, k)
			n, err := br.ReadAt(p, off)
			a = fmt.Sprintf("ReadAt %d %v %q", n, err, p[:n])
			q := make([]byte, k)
			n, err = sr.ReadAt(q, off)
			c = fmt.Sprintf("ReadAt %d %v %q", n, err, q[:n])
		case 2:
			x, err := br.ReadByte()
			a = fmt.Sprint("ReadByte ", x, err)
			x, err = sr.ReadByte()
			c = fmt.Sprint("ReadByte ", x, err)
		case 3:
			a = fmt.Sprint("UnreadByte ", br.UnreadByte())
			c = fmt.Sprint("UnreadByte ", sr.UnreadByte())
		case 4:
			r, size, err := br.ReadRune()
			a = fmt.Sprint("ReadRune ", r, size, err)
			r, size, err = sr.ReadRune()
			c = fmt.Sprint("ReadRune ", r, size, err)
		case 5:
			a = fmt.Sprint("UnreadRune ", br.UnreadRune() == nil)
			c = fmt.Sprint("UnreadRune ", sr.UnreadRune() == nil)
		case 6:
			off := int64(pick(len(input)+4)) - 2
			whence := pick(4)
			pos, err := br.Seek(off, whence)
			a = fmt.Sprint("Seek ", pos, err == nil)
			pos, err = sr.Seek(off, whence)
			c = fmt.Sprint("Seek ", pos, err == nil)
		case 7:
			var x, y bytes.Buffer
			n, err := br.WriteTo(&x)
			a = fmt.Sprintf("WriteTo %d %v %q", n, err, x.String())
			n, err = sr.WriteTo(&y)
			c = fmt.Sprintf("WriteTo %d %v %q", n, err, y.String())
		}
		fmt.Println(a, "|", br.Len(), a == c, sr.Len())
	}
}

func main() {
	buffers()
	bytesReader()
	stringsReader()
	randomBuffer()
	randomReaders()
}
