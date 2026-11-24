package main

import (
	"encoding/binary"
	"fmt"
)

func roundTrip(order binary.ByteOrder) {
	b := make([]byte, 8)
	order.PutUint16(b, 0x0102)
	v16 := order.Uint16(b)
	order.PutUint32(b, 0x01020304)
	v32 := order.Uint32(b)
	order.PutUint64(b, 0x0102030405060708)
	v64 := order.Uint64(b)
	fmt.Println(order, b, v16, v32, v64)
}

func appendAll(order binary.AppendByteOrder) {
	var b []byte
	b = order.AppendUint16(b, 0xabcd)
	b = order.AppendUint32(b, 0x01020304)
	b = order.AppendUint64(b, 0x1122334455667788)
	fmt.Println(order.String(), b)
}

func uvarint(buf []byte) {
	x, n := binary.Uvarint(buf)
	y, m := binary.Varint(buf)
	fmt.Println(buf, x, n, y, m)
}

func attempt(name string, f func()) {
	defer func() {
		fmt.Println(name, "recovered:", recover())
	}()
	f()
}

func main() {
	b := []byte{1, 2, 3, 4, 5, 6, 7, 8}
	fmt.Println(binary.BigEndian.Uint16(b), binary.LittleEndian.Uint16(b), binary.BigEndian.Uint32(b), binary.LittleEndian.Uint32(b))
	fmt.Println(binary.BigEndian.Uint64(b), binary.LittleEndian.Uint64(b))
	binary.BigEndian.PutUint32(b[4:], 0xdeadbeef)
	binary.LittleEndian.PutUint16(b, 0xbeef)
	fmt.Println(b)
	roundTrip(binary.BigEndian)
	roundTrip(binary.LittleEndian)
	appendAll(binary.BigEndian)
	appendAll(binary.LittleEndian)
	out := binary.BigEndian.AppendUint64([]byte{9}, 1)
	fmt.Println(out, binary.BigEndian, binary.LittleEndian.GoString())
	fmt.Printf("%v %s %T %T\n", binary.BigEndian, binary.LittleEndian, binary.BigEndian, binary.LittleEndian)

	// Varints, with the lengths of each size and the overflow and short-buffer results.
	for _, x := range []uint64{0, 1, 127, 128, 300, 16383, 16384, 1<<63 - 1, 1 << 63, 18446744073709551615} {
		buf := make([]byte, binary.MaxVarintLen64)
		n := binary.PutUvarint(buf, x)
		y, m := binary.Uvarint(buf[:n])
		fmt.Printf("%v %v %v | ", n, y == x, m)
	}
	fmt.Println()
	for _, x := range []int64{0, -1, 1, -64, 64, -65, 1<<62, -(1 << 62)} {
		buf := make([]byte, binary.MaxVarintLen64)
		n := binary.PutVarint(buf, x)
		y, m := binary.Varint(buf[:n])
		fmt.Printf("%v %v %v | ", buf[:n], y, m)
	}
	fmt.Println()
	uvarint(nil)
	uvarint([]byte{0x80})
	uvarint([]byte{0xac, 0x02, 0xff})
	uvarint([]byte{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})
	uvarint([]byte{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02})
	uvarint([]byte{0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01})
	fmt.Println(binary.AppendUvarint(nil, 300), binary.AppendVarint([]byte{7}, -3), binary.MaxVarintLen16, binary.MaxVarintLen32)

	attempt("uint64", func() { binary.BigEndian.Uint64(b[:7]) })
	attempt("put", func() { binary.LittleEndian.PutUint32(b[6:], 1) })
	attempt("uvarint", func() { binary.PutUvarint(make([]byte, 1), 300) })
	attempt("nil", func() { binary.BigEndian.Uint16(nil) })
}
