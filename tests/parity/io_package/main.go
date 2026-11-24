package main

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"strings"
)

// A Reader that returns at most k bytes per Read and is not a WriterTo, so that io.Copy takes its
// generic loop.
type chunkReader struct {
	data []byte
	k    int
}

func (c *chunkReader) Read(p []byte) (int, error) {
	if len(c.data) == 0 {
		return 0, io.EOF
	}
	n := c.k
	if n > len(p) {
		n = len(p)
	}
	if n > len(c.data) {
		n = len(c.data)
	}
	copy(p, c.data[:n])
	c.data = c.data[n:]
	return n, nil
}

// A Reader that returns its data with io.EOF on the same call.
type eofWithData struct {
	data string
	done bool
}

func (e *eofWithData) Read(p []byte) (int, error) {
	if e.done {
		return 0, io.EOF
	}
	e.done = true
	n := copy(p, e.data)
	return n, io.EOF
}

// A Writer that accepts at most limit bytes in total and then reports a short write.
type shortWriter struct {
	got   []byte
	limit int
}

func (w *shortWriter) Write(p []byte) (int, error) {
	n := len(p)
	if n > w.limit-len(w.got) {
		n = w.limit - len(w.got)
	}
	w.got = append(w.got, p[:n]...)
	return n, nil
}

var errBroken = errors.New("broken")

// A Writer that fails after the first write.
type failWriter struct {
	calls int
}

func (w *failWriter) Write(p []byte) (int, error) {
	w.calls++
	if w.calls > 1 {
		return 0, errBroken
	}
	return len(p), nil
}

// A Writer that only has Write, so io.WriteString converts the string.
type plainWriter struct {
	got string
}

func (w *plainWriter) Write(p []byte) (int, error) {
	w.got += string(p)
	return len(p), nil
}

func readFull() {
	buf := make([]byte, 4)
	n, err := io.ReadFull(strings.NewReader("abcdef"), buf)
	fmt.Println(n, err, string(buf[:n]))
	n, err = io.ReadFull(strings.NewReader("ab"), buf)
	fmt.Println(n, err, err == io.ErrUnexpectedEOF)
	n, err = io.ReadFull(strings.NewReader(""), buf)
	fmt.Println(n, err, err == io.EOF)
	n, err = io.ReadFull(&chunkReader{[]byte("wxyz!"), 1}, buf)
	fmt.Println(n, err, string(buf[:n]))
	n, err = io.ReadAtLeast(strings.NewReader("abcdef"), buf, 2)
	fmt.Println(n, err, string(buf[:n]))
	n, err = io.ReadAtLeast(strings.NewReader("abc"), buf, 5)
	fmt.Println(n, err, err == io.ErrShortBuffer)
	n, err = io.ReadAtLeast(&eofWithData{"xyz", false}, buf, 3)
	fmt.Println(n, err)
	n, err = io.ReadFull(strings.NewReader("abc"), nil)
	fmt.Println(n, err)
}

func readAll() {
	b, err := io.ReadAll(strings.NewReader(""))
	fmt.Println(len(b), b == nil, err)
	b, err = io.ReadAll(&chunkReader{[]byte(strings.Repeat("0123456789", 300)), 7})
	fmt.Println(len(b), err, string(b[:12]), string(b[len(b)-5:]))
	b, err = io.ReadAll(&eofWithData{"tail", false})
	fmt.Println(string(b), err)
	b, err = io.ReadAll(io.LimitReader(strings.NewReader("limited reader"), 7))
	fmt.Println(string(b), err)
}

func copies() {
	var dst bytes.Buffer
	n, err := io.Copy(&dst, strings.NewReader("via WriterTo"))
	fmt.Println(n, err, dst.String())
	dst.Reset()
	n, err = io.Copy(&dst, &chunkReader{[]byte("via ReaderFrom"), 3})
	fmt.Println(n, err, dst.String())
	pw := &plainWriter{}
	n, err = io.Copy(pw, &chunkReader{[]byte("generic loop"), 5})
	fmt.Println(n, err, pw.got)
	sw := &shortWriter{nil, 4}
	n, err = io.Copy(sw, &chunkReader{[]byte("short write"), 3})
	fmt.Println(n, err, err == io.ErrShortWrite, string(sw.got))
	fw := &failWriter{}
	n, err = io.Copy(fw, &chunkReader{[]byte("abcdef"), 2})
	fmt.Println(n, err, fw.calls)
	pw = &plainWriter{}
	n, err = io.CopyN(pw, strings.NewReader("copy n bytes"), 6)
	fmt.Println(n, err, pw.got)
	pw = &plainWriter{}
	n, err = io.CopyN(pw, strings.NewReader("abc"), 6)
	fmt.Println(n, err, pw.got)
	pw = &plainWriter{}
	n, err = io.CopyBuffer(pw, &chunkReader{[]byte("with a buffer"), 100}, make([]byte, 2))
	fmt.Println(n, err, pw.got)
	n, err = io.Copy(io.Discard, &chunkReader{[]byte("discarded"), 4})
	fmt.Println(n, err)
	n2, err := io.WriteString(io.Discard, "gone")
	fmt.Println(n2, err)
	pw = &plainWriter{}
	n2, err = io.WriteString(pw, "plain")
	fmt.Println(n2, err, pw.got)
	var sb bytes.Buffer
	n2, err = io.WriteString(&sb, "string writer")
	fmt.Println(n2, err, sb.String())
}

func limits() {
	lr := &io.LimitedReader{R: strings.NewReader("0123456789"), N: 4}
	buf := make([]byte, 3)
	for {
		n, err := lr.Read(buf)
		fmt.Println(n, err, string(buf[:n]), lr.N)
		if err != nil {
			break
		}
	}
	lr = &io.LimitedReader{R: strings.NewReader("abc"), N: -1}
	n, err := lr.Read(buf)
	fmt.Println(n, err)
}

func multi() {
	r := io.MultiReader(strings.NewReader("one "), strings.NewReader(""), &chunkReader{[]byte("two "), 2}, bytes.NewReader([]byte("three")))
	buf := make([]byte, 3)
	var got []string
	for {
		n, err := r.Read(buf)
		got = append(got, fmt.Sprintf("%d:%q:%v", n, buf[:n], err))
		if err != nil {
			break
		}
	}
	fmt.Println(strings.Join(got, " "))
	n, err := r.Read(buf)
	fmt.Println(n, err)

	inner := io.MultiReader(strings.NewReader("a"), strings.NewReader("b"))
	outer := io.MultiReader(inner, strings.NewReader("c"), io.MultiReader(strings.NewReader("d")))
	var dst bytes.Buffer
	n64, err := io.Copy(&dst, outer)
	fmt.Println(n64, err, dst.String())
	b, err := io.ReadAll(io.MultiReader())
	fmt.Println(len(b), err)

	var x, y bytes.Buffer
	pw := &plainWriter{}
	w := io.MultiWriter(&x, io.MultiWriter(&y, pw))
	n, err = w.Write([]byte("fan "))
	fmt.Println(n, err)
	n, err = io.WriteString(w, "out")
	fmt.Println(n, err, x.String(), y.String(), pw.got)
	sw := &shortWriter{nil, 2}
	w = io.MultiWriter(&x, sw, &y)
	n, err = w.Write([]byte("xyz"))
	fmt.Println(n, err, x.String(), y.String())
	fw := &failWriter{1}
	n, err = io.MultiWriter(fw).Write([]byte("q"))
	fmt.Println(n, err)
}

func tee() {
	var log bytes.Buffer
	r := io.TeeReader(&chunkReader{[]byte("teed data"), 4}, &log)
	b, err := io.ReadAll(r)
	fmt.Println(string(b), err, log.String())
	fw := &failWriter{1}
	r = io.TeeReader(strings.NewReader("abc"), fw)
	n, err := r.Read(make([]byte, 2))
	fmt.Println(n, err)
}

func sections() {
	base := strings.NewReader("0123456789abcdef")
	s := io.NewSectionReader(base, 4, 6)
	fmt.Println(s.Size())
	buf := make([]byte, 4)
	n, err := s.Read(buf)
	fmt.Println(n, err, string(buf[:n]))
	n, err = s.Read(buf)
	fmt.Println(n, err, string(buf[:n]))
	n, err = s.Read(buf)
	fmt.Println(n, err)
	pos, err := s.Seek(1, io.SeekStart)
	fmt.Println(pos, err)
	pos, err = s.Seek(2, io.SeekCurrent)
	fmt.Println(pos, err)
	pos, err = s.Seek(-1, io.SeekEnd)
	fmt.Println(pos, err)
	pos, err = s.Seek(-10, io.SeekCurrent)
	fmt.Println(pos, err)
	pos, err = s.Seek(0, 7)
	fmt.Println(pos, err)
	n, err = s.ReadAt(buf, 3)
	fmt.Println(n, err, string(buf[:n]))
	n, err = s.ReadAt(buf, 1)
	fmt.Println(n, err, string(buf[:n]))
	n, err = s.ReadAt(buf, 6)
	fmt.Println(n, err)
	n, err = s.ReadAt(buf, -1)
	fmt.Println(n, err)
	r, off, size := s.Outer()
	fmt.Println(r == io.ReaderAt(base), off, size)
	big := io.NewSectionReader(base, 2, 1<<62+(1<<62-1))
	b, err := io.ReadAll(big)
	fmt.Println(string(b), err, big.Size())
}

func nopClosers() {
	rc := io.NopCloser(strings.NewReader("closer"))
	_, isWriterTo := rc.(io.WriterTo)
	fmt.Println(rc.Close(), isWriterTo)
	var dst bytes.Buffer
	n, err := io.Copy(&dst, rc)
	fmt.Println(n, err, dst.String())
	rc = io.NopCloser(&chunkReader{[]byte("plain"), 2})
	_, isWriterTo = rc.(io.WriterTo)
	b, err := io.ReadAll(rc)
	fmt.Println(string(b), err, isWriterTo, rc.Close())
}

func errorValues() {
	fmt.Println(io.EOF, io.ErrUnexpectedEOF, io.ErrShortWrite, io.ErrShortBuffer)
	fmt.Println(io.ErrNoProgress, io.ErrClosedPipe)
	fmt.Println(io.SeekStart, io.SeekCurrent, io.SeekEnd)
	wrapped := fmt.Errorf("reading: %w", io.EOF)
	fmt.Println(wrapped, errors.Is(wrapped, io.EOF), wrapped == io.EOF)
}

func attempt(name string, f func()) {
	defer func() { fmt.Println(name, "recovered:", recover()) }()
	f()
}

func main() {
	readFull()
	readAll()
	copies()
	limits()
	multi()
	tee()
	sections()
	nopClosers()
	errorValues()
	attempt("empty buffer", func() { io.CopyBuffer(io.Discard, strings.NewReader("x"), make([]byte, 0)) })
	attempt("nil reader", func() { io.ReadAll(nil) })
}
