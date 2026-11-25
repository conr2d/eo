package main

import (
	"bufio"
	"bytes"
	"errors"
	"fmt"
	"io"
	"strings"
	"unicode/utf8"
)

// oneByte returns at most one byte per Read, as testing/iotest.OneByteReader does.
type oneByte struct {
	r io.Reader
}

func (o *oneByte) Read(p []byte) (int, error) {
	if len(p) == 0 {
		return 0, nil
	}
	return o.r.Read(p[0:1])
}

// limited accepts at most max bytes per Write and then fails.
type limited struct {
	buf bytes.Buffer
	max int
}

func (l *limited) Write(p []byte) (int, error) {
	if len(p) > l.max {
		l.buf.Write(p[:l.max])
		return l.max, errors.New("limited: full")
	}
	l.buf.Write(p)
	return len(p), nil
}

// counting counts its Write calls, and it is not an io.StringWriter.
type counting struct {
	calls int
	data  []byte
}

func (c *counting) Write(p []byte) (int, error) {
	c.calls++
	c.data = append(c.data, p...)
	return len(p), nil
}

type bufferedI interface {
	Buffered() int
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func utf8Cases() {
	inputs := []string{"", "a", "é", "€", "𝄞", "\xff", "\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\xf0\x9d\x84", "a€𝄞\xff"}
	for _, s := range inputs {
		b := []byte(s)
		r, n := utf8.DecodeRune(b)
		rs, ns := utf8.DecodeRuneInString(s)
		lr, ln := utf8.DecodeLastRune(b)
		lrs, lns := utf8.DecodeLastRuneInString(s)
		fmt.Printf("%q %U %d %U %d %U %d %U %d %v %v %d %d %v %v\n", s, r, n, rs, ns, lr, ln, lrs, lns,
			utf8.FullRune(b), utf8.FullRuneInString(s), utf8.RuneCount(b), utf8.RuneCountInString(s),
			utf8.Valid(b), utf8.ValidString(s))
	}
	for _, r := range []rune{-1, 0, 0x7f, 0x80, 0x7ff, 0x800, 0xd7ff, 0xd800, 0xdfff, 0xe000, 0xffff, 0x10000, 0x10ffff, 0x110000} {
		p := make([]byte, 4)
		n := utf8.EncodeRune(p, r)
		fmt.Println(r, utf8.RuneLen(r), utf8.ValidRune(r), n, p[:n], utf8.AppendRune([]byte("x"), r))
	}
	fmt.Println(utf8.RuneStart(0x80), utf8.RuneStart('a'), utf8.RuneStart(0xe2), utf8.RuneError, utf8.RuneSelf, utf8.MaxRune, utf8.UTFMax)
	short_ := []byte{1, 2}
	attempt("EncodeRune short", func() { utf8.EncodeRune(short_, '€') })
	fmt.Println(short_)
	fmt.Println(utf8.AppendRune(nil, 'a'), utf8.AppendRune(nil, -5))
}

func readerCases() {
	// derive.BatchReader: peek the compression byte, discard it, read the rest as an io.ByteReader.
	br := bufio.NewReader(&oneByte{strings.NewReader("\x01payload")})
	ct, err := br.Peek(1)
	fmt.Println(ct, err, br.Buffered())
	d, err := br.Discard(1)
	fmt.Println(d, err, br.Buffered())
	var byteReader io.ByteReader = br
	var sb strings.Builder
	for {
		c, err := byteReader.ReadByte()
		if err != nil {
			fmt.Println(sb.String(), err, err == io.EOF)
			break
		}
		sb.WriteByte(c)
	}

	r := bufio.NewReaderSize(strings.NewReader("0123456789abcdefghijklmnopqrstuvwxyz"), 4)
	fmt.Println(r.Size())
	p, err := r.Peek(20)
	fmt.Println(string(p), err)
	p, err = r.Peek(-1)
	fmt.Println(p == nil, err)
	d, err = r.Discard(-1)
	fmt.Println(d, err)
	d, err = r.Discard(0)
	fmt.Println(d, err)
	d, err = r.Discard(20)
	fmt.Println(d, err, r.Buffered())
	c, err := r.ReadByte()
	fmt.Println(string(c), err, r.UnreadByte(), r.UnreadByte())
	c, _ = r.ReadByte()
	fmt.Println(string(c))
	buf := make([]byte, 40)
	n, err := r.Read(buf)
	fmt.Println(n, string(buf[:n]), err)
	n, err = r.Read(buf)
	fmt.Println(n, string(buf[:n]), err)
	d, err = r.Discard(100)
	fmt.Println(d, err)
	n, err = r.Read(buf)
	fmt.Println(n, err)
	p, err = r.Peek(1)
	fmt.Println(p, err)

	same := bufio.NewReaderSize(r, 4)
	bigger := bufio.NewReaderSize(r, 64)
	fmt.Println(same == r, bigger == r, bigger.Size())
	r.Reset(r)
	r.Reset(strings.NewReader("héllo, 世界\xff!"))
	for {
		ch, size, err := r.ReadRune()
		if err != nil {
			fmt.Println("rune end:", err)
			break
		}
		fmt.Printf("%q %d ", ch, size)
	}
	fmt.Println()
	fmt.Println(r.UnreadRune(), r.UnreadByte())

	r = bufio.NewReader(strings.NewReader("ab"))
	ch, size, _ := r.ReadRune()
	fmt.Println(string(ch), size, r.UnreadRune(), r.UnreadRune())
	ch, size, _ = r.ReadRune()
	fmt.Println(string(ch), size)
	fmt.Println(r.UnreadByte())
	ch, size, _ = r.ReadRune()
	fmt.Println(string(ch), size)

	lines := "short\nwindows\r\nthis line is longer than sixteen bytes\nsplit\r" + strings.Repeat("x", 15) + "\r\nlast"
	r = bufio.NewReaderSize(strings.NewReader(lines), 16)
	for {
		line, isPrefix, err := r.ReadLine()
		fmt.Printf("%q %v %v\n", line, isPrefix, err)
		if err != nil {
			break
		}
	}

	r = bufio.NewReaderSize(strings.NewReader("abc,defghijklmnopqrstuvwxyz,,tail"), 16)
	for {
		s, err := r.ReadSlice(',')
		fmt.Printf("%q %v\n", s, err)
		if err != nil && err != bufio.ErrBufferFull {
			break
		}
	}

	r = bufio.NewReaderSize(strings.NewReader(strings.Repeat("0123456789", 5)+";next;end"), 16)
	b, err := r.ReadBytes(';')
	fmt.Println(len(b), string(b[45:]), err)
	s, err := r.ReadString(';')
	fmt.Printf("%q %v\n", s, err)
	s, err = r.ReadString(';')
	fmt.Printf("%q %v\n", s, err)
	b, err = r.ReadBytes(';')
	fmt.Println(b == nil, len(b), err)

	r = bufio.NewReaderSize(strings.NewReader("write to buffer"), 16)
	r.Peek(5)
	var out bytes.Buffer
	m, err := r.WriteTo(&out)
	fmt.Println(m, err, out.String())

	r = bufio.NewReaderSize(&oneByte{strings.NewReader("through a plain writer")}, 16)
	r.ReadByte()
	cw := &counting{}
	m, err = r.WriteTo(cw)
	fmt.Println(m, err, string(cw.data), cw.calls > 1)

	r = bufio.NewReaderSize(strings.NewReader("big read goes straight through"), 16)
	n, err = r.Read(buf)
	fmt.Println(n, string(buf[:n]), err, r.UnreadByte())
	c, err = r.ReadByte()
	fmt.Println(c, err)

	var nilBuf bufio.Reader
	nilBuf.Reset(strings.NewReader("zero"))
	fmt.Println(nilBuf.Size())
	p, err = nilBuf.Peek(4)
	fmt.Println(string(p), err)

	_, isByteScanner := any(r).(io.ByteScanner)
	_, isRuneScanner := any(r).(io.RuneScanner)
	_, isWriterTo := any(r).(io.WriterTo)
	_, isReaderFrom := any(r).(io.ReaderFrom)
	fmt.Println(isByteScanner, isRuneScanner, isWriterTo, isReaderFrom)
	fmt.Printf("%T\n", r)
}

func writerCases() {
	var out bytes.Buffer
	w := bufio.NewWriterSize(&out, 8)
	fmt.Println(w.Size(), w.Available(), w.Buffered())
	w.WriteString("abc")
	fmt.Println(w.Available(), w.Buffered(), out.Len())
	n, err := w.Write([]byte("defghijklmnopqrstu"))
	fmt.Println(n, err, w.Buffered(), out.String())
	fmt.Println(w.Flush(), out.String(), w.Buffered())
	fmt.Println(w.Flush())
	w.WriteByte('!')
	sz, err := w.WriteRune('€')
	fmt.Println(sz, err, w.Buffered())
	w.WriteString("1234")
	sz, err = w.WriteRune('𝄞')
	fmt.Println(sz, err, w.Buffered(), out.String())
	w.Flush()
	fmt.Printf("%q\n", out.String())

	tiny := bufio.NewWriterSize(&out, 2)
	fmt.Println(tiny.Size())
	sz, err = tiny.WriteRune('世')
	fmt.Println(sz, err, tiny.Buffered())
	tiny.Flush()
	fmt.Println(strings.HasSuffix(out.String(), "世"))

	ab := w.AvailableBuffer()
	fmt.Println(len(ab), cap(ab))
	ab = append(ab, "xyz"...)
	w.Write(ab)
	fmt.Println(w.Buffered())
	w.Flush()

	cw := &counting{}
	w = bufio.NewWriterSize(cw, 4)
	n, err = w.WriteString("a long string that skips the buffer")
	fmt.Println(n, err, cw.calls, w.Buffered())
	w.Flush()
	fmt.Println(string(cw.data), cw.calls)

	var sb strings.Builder
	w = bufio.NewWriterSize(&sb, 4)
	n, err = w.WriteString("straight into a string writer")
	fmt.Println(n, err, sb.String(), w.Buffered())

	l := &limited{max: 5}
	w = bufio.NewWriterSize(l, 4)
	w.WriteString("abc")
	n, err = w.WriteString("defghij")
	fmt.Println(n, err, l.buf.String(), w.Buffered())
	fmt.Println(w.Flush(), w.WriteByte('z'))
	sz, err = w.WriteRune('x')
	fmt.Println(sz, err)
	n, err = w.Write([]byte("more"))
	fmt.Println(n, err)
	w.Reset(&out)
	fmt.Println(w.Buffered(), w.Flush())

	l = &limited{max: 2}
	w = bufio.NewWriterSize(l, 8)
	w.WriteString("abcdef")
	fmt.Println(w.Flush(), w.Buffered(), l.buf.String())

	out.Reset()
	w = bufio.NewWriterSize(&out, 4)
	w.WriteString("xy")
	m, err := w.ReadFrom(strings.NewReader("read from a reader"))
	fmt.Println(m, err, out.String(), w.Buffered())
	w.Flush()
	fmt.Println(out.String())

	cw = &counting{}
	w = bufio.NewWriterSize(cw, 4)
	m, err = w.ReadFrom(&oneByte{strings.NewReader("one at a time")})
	fmt.Println(m, err, w.Buffered())
	w.Flush()
	fmt.Println(string(cw.data))

	same := bufio.NewWriterSize(w, 4)
	bigger := bufio.NewWriterSize(w, 16)
	def := bufio.NewWriterSize(&out, 0)
	fmt.Println(same == w, bigger == w, def.Size(), bufio.NewWriter(&out).Size())
	w.Reset(w)

	var zero bufio.Writer
	zero.Reset(&out)
	fmt.Println(zero.Size())

	_, isWriter := any(w).(io.Writer)
	_, isByteWriter := any(w).(io.ByteWriter)
	_, isStringWriter := any(w).(io.StringWriter)
	_, isReaderFrom := any(w).(io.ReaderFrom)
	fmt.Println(isWriter, isByteWriter, isStringWriter, isReaderFrom)
}

func readWriterCases() {
	var out bytes.Buffer
	rw := bufio.NewReadWriter(bufio.NewReader(strings.NewReader("in\nput")), bufio.NewWriter(&out))
	line, err := rw.ReadString('\n')
	fmt.Printf("%q %v\n", line, err)
	rw.WriteString("out")
	rw.Flush()
	fmt.Println(out.String(), rw.Reader.Buffered(), rw.Writer.Buffered())
	var iorw io.ReadWriter = rw
	buf := make([]byte, 8)
	n, err := iorw.Read(buf)
	fmt.Println(n, string(buf[:n]), err)
	_, hasBuffered := any(rw).(bufferedI)
	_, isByteScanner := any(rw).(io.ByteScanner)
	fmt.Println(hasBuffered, isByteScanner)
}

func scannerCases() {
	s := bufio.NewScanner(strings.NewReader("one\ntwo\r\n\nthree"))
	for s.Scan() {
		fmt.Printf("%q ", s.Text())
	}
	fmt.Println(s.Err())

	s = bufio.NewScanner(strings.NewReader("  the quick\tbrown fox jumps  "))
	s.Split(bufio.ScanWords)
	for s.Scan() {
		fmt.Printf("%q ", s.Bytes())
	}
	fmt.Println(s.Err())

	s = bufio.NewScanner(&oneByte{strings.NewReader("a€\xff𝄞\xe2\x82")})
	s.Split(bufio.ScanRunes)
	for s.Scan() {
		fmt.Printf("%q ", s.Text())
	}
	fmt.Println(s.Err())

	s = bufio.NewScanner(strings.NewReader("xyz"))
	s.Split(bufio.ScanBytes)
	for s.Scan() {
		fmt.Print(s.Bytes(), " ")
	}
	fmt.Println(s.Err())

	s = bufio.NewScanner(strings.NewReader("a,b,STOP,c"))
	s.Split(func(data []byte, atEOF bool) (int, []byte, error) {
		i := bytes.IndexByte(data, ',')
		if i < 0 {
			if atEOF && len(data) > 0 {
				return len(data), data, nil
			}
			return 0, nil, nil
		}
		if string(data[:i]) == "STOP" {
			return i + 1, []byte("stopped"), bufio.ErrFinalToken
		}
		return i + 1, data[:i], nil
	})
	for s.Scan() {
		fmt.Printf("%q ", s.Text())
	}
	fmt.Println(s.Err(), s.Scan())

	s = bufio.NewScanner(strings.NewReader("short\n" + strings.Repeat("y", 40) + "\nafter"))
	s.Buffer(make([]byte, 0, 8), 16)
	for s.Scan() {
		fmt.Printf("%q ", s.Text())
	}
	fmt.Println(s.Err(), s.Err() == bufio.ErrTooLong)

	s = bufio.NewScanner(strings.NewReader("abc"))
	s.Split(func(data []byte, atEOF bool) (int, []byte, error) { return -1, nil, nil })
	fmt.Println(s.Scan(), s.Err())

	s = bufio.NewScanner(strings.NewReader("abc"))
	s.Split(func(data []byte, atEOF bool) (int, []byte, error) { return len(data) + 1, nil, nil })
	fmt.Println(s.Scan(), s.Err())

	s = bufio.NewScanner(strings.NewReader("abc"))
	s.Split(func(data []byte, atEOF bool) (int, []byte, error) { return 0, nil, errors.New("split failed") })
	fmt.Println(s.Scan(), s.Err())

	s = bufio.NewScanner(strings.NewReader("a\nb"))
	s.Scan()
	attempt("Split after Scan", func() { s.Split(bufio.ScanWords) })
	attempt("Buffer after Scan", func() { s.Buffer(nil, 10) })

	s = bufio.NewScanner(strings.NewReader(""))
	fmt.Println(s.Scan(), s.Err(), s.Bytes() == nil)

	adv, tok, err := bufio.ScanLines([]byte("no newline"), false)
	fmt.Println(adv, tok == nil, err)
	adv, tok, err = bufio.ScanLines([]byte("cr\r"), true)
	fmt.Printf("%d %q %v\n", adv, tok, err)
	adv, tok, err = bufio.ScanWords([]byte("   "), false)
	fmt.Println(adv, tok == nil, err)
	adv, tok, err = bufio.ScanRunes([]byte("\xe2\x82"), false)
	fmt.Println(adv, tok == nil, err)
	fmt.Println(bufio.MaxScanTokenSize, bufio.ErrBufferFull, bufio.ErrNegativeCount, bufio.ErrInvalidUnreadByte, bufio.ErrInvalidUnreadRune)
	fmt.Println(bufio.ErrNegativeAdvance, bufio.ErrAdvanceTooFar, bufio.ErrBadReadCount, bufio.ErrFinalToken)
}

func main() {
	utf8Cases()
	readerCases()
	writerCases()
	readWriterCases()
	scannerCases()
}
